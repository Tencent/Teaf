#include "isgw_intf.h"
#include "isgw_mgr_svc.h"
#include "stat.h"
#include "isgw_ack.h"
#include "cmd_amount_contrl.h"

int ISGWIntf::msg_seq_ = 0;

ISGWIntf::ISGWIntf() : recv_len_(0), msg_len_(0)
{
    //memset(recv_buf_, 0x0, sizeof(recv_buf_));
}

ISGWIntf::~ISGWIntf()
{
    
}

int ISGWIntf::open(void* p)
{
    if (AceSockHdrBase::open(p) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWIntf open failed,ip=%s\n"
            , remote_addr_.get_host_addr()));
        return -1;
    }
    
    //ip权限检验暂时取消
    if (is_auth() != 0)
    {
        return -1;
    }
    
#ifdef MAX_IDLE_TIME_SEC    
    // 因为需要清理无效连接 加上定时器机制 
    ACE_Time_Value delay(MAX_IDLE_TIME_SEC,0);
    ACE_Time_Value interval(MAX_IDLE_TIME_SEC,0); //10 分钟 
    ACE_Reactor::instance()->schedule_timer(this, 0, delay, interval);
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWIntf start clean timer,interval=%d\n", MAX_IDLE_TIME_SEC));
#endif
    
    return 0;
}

#ifndef MSG_LEN_SIZE  
//纯文本协议，以特殊字符作为结束符
int ISGWIntf::handle_input(ACE_HANDLE fd) //= ACE_INVALID_HANDLE
{
    lastrtime_ = ISGWAck::instance()->get_time();
    //接收消息
    int ret = this->peer().recv((char*)recv_buf_ + recv_len_,
                                MAX_RECV_BUF_LEN - recv_len_); //, &timeout    
    switch(ret)
    {
        case -1:
        {
            ACE_DEBUG((LM_ERROR, "[%D] ISGWIntf recv failed"
                ",ret=%d"
                ",errno=%d"
                ",errmsg=%s"
                ",ip=%s"
                ",fd=%d"
                ",handle=%d"
                "\n"
                , ret
                , errno
                , strerror(errno)
                , remote_addr_.get_host_addr()
                , fd
                , get_handle()
                ));
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS)
            {
                return 0;
            }
            return -1;
        }
        break;
        case 0:
        {
            ACE_DEBUG((LM_NOTICE, "[%D] ISGWIntf recv succ"
                ",connection closed by foreign host"
				",ret=%d,ip=%s\n"
				, ret, remote_addr_.get_host_addr()
				));
            return -1;
        }
        break;
        default: //接收成功
        recv_len_ += ret;
        ACE_DEBUG((LM_NOTICE, "[%D] ISGWIntf recv succ,recv_len=%d,ret=%d,recv_buf=%s\n"
            , recv_len_, ret, recv_buf_));

        //粗略判断消息是否结束(即是否已经有一个完整的消息包)
        //因为此处判断可能包括了垃圾数据里面的结束符
        if ( strstr(recv_buf_, MSG_SEPARATOR) == NULL) //未结束，继续接收
        {
            if ( recv_len_ < MAX_RECV_BUF_LEN )
            {
                ACE_DEBUG((LM_WARNING, "[%D] ISGWIntf recv succ,no end"
                    ",recv_len=%d,recv_buf=%s\n"
                    , recv_len_, recv_buf_));
                return 0;
            }
            else
            {
                ACE_DEBUG((LM_ERROR, "[%D] ISGWIntf recv failed,illegal msg"
                    ",reach max buff len and have no msg end,discard it,ip=%s\n"
                    , remote_addr_.get_host_addr()
                    ));
                return -1; //消息非法
            }
        }
		
    }
    
    //已经获得完整的消息
    
    //判断消息是否非法
    if (is_legal(recv_buf_) != 0)
    {
    	ACE_DEBUG((LM_ERROR, "[%D] ISGWIntf recv failed,illegal msg,discard it"
            ",ip=%s\n", remote_addr_.get_host_addr()
            ));
    	recv_len_ = 0; // reset the recv pos indicator 
    	//memset(recv_buf_, 0x0, sizeof(recv_buf_));
    	return 0;
    }

    char *msg_start = recv_buf_;
    char *msg_end = NULL;
    int proced_len = 0; //已经处理的长度
    int pend_len = recv_len_; //pend_len 未处理的消息长度

    //proced_len 必须小于recv_len_ 避免处理到了buf后面的垃圾数据 
    while ((msg_end = strstr(msg_start, MSG_SEPARATOR)) != NULL && proced_len < recv_len_)
    {
        msg_len_ = msg_end + strlen(MSG_SEPARATOR)-msg_start;
        // 看看是否会处理到后面的垃圾数据(不完整的消息)
        if((proced_len+msg_len_) > recv_len_)
        {
            ACE_DEBUG((LM_WARNING, "[%D] ISGWIntf proc no end msg"
                ",proced_len=%d,msg_len_=%d,recv_len_=%d\n"
                , proced_len, msg_len_, recv_len_
                ));
            break;
        }
        if (msg_len_ > strlen(MSG_SEPARATOR) && process(msg_start, get_handle(), get_seq(), msg_len_) != 0)
        {
            //如果解析或者处理出错(后端无处理能力),整个消息一起丢弃,并导致断开连接    		
            return -1;
        }
        
        proced_len += msg_len_;
        pend_len -= msg_len_;
        msg_start = msg_end + strlen(MSG_SEPARATOR);  
        
        ACE_DEBUG((LM_TRACE, "[%D] ISGWIntf proc msg succ,msg_len=%d,proced_len=%d,pend_len=%d\n"
			, msg_len_, proced_len, pend_len));
    }
    
    //移动剩下的数据
    if (msg_start!=recv_buf_)
    {
        memmove(recv_buf_, msg_start, pend_len);
        ACE_DEBUG((LM_TRACE, "[%D] ISGWIntf proc msg move,start=%d,pend_len=%d\n"
            , (msg_start-recv_buf_), pend_len));
    }
    
    //memset(recv_buf_+pend_len, 0x0, sizeof(recv_buf_)-pend_len);
    recv_len_ = pend_len; // reset the recv pos indicator    
    
    return 0;
}

#else
// 消息头指定长度的消息
int ISGWIntf::handle_input(ACE_HANDLE /*fd = ACE_INVALID_HANDLE*/)
{
    lastrtime_ = ISGWAck::instance()->get_time();
    // 文本协议,长度字段在消息中额外占MSG_LEN_SIZE字节
    unsigned int len_field_extra_size = MSG_LEN_SIZE;          
#ifdef BINARY_PROTOCOL
    // 二进制协议,长度字段本身包含在消息中
    len_field_extra_size = 0;                                  
#endif 

    //接收消息
    int ret = this->peer().recv((char*)recv_buf_ + recv_len_,
                                MAX_RECV_BUF_LEN - recv_len_); //, &timeout
    switch(ret)
    {
        case -1:
        {  
            ACE_DEBUG((LM_ERROR, "[%D] ISGWIntf recv failed"
            	",ret=%d"
                ",errno=%d"
                ",errmsg=%s"
                ",ip=%s"
                "\n"
                , ret
                , errno
                , strerror(errno)
                , remote_addr_.get_host_addr()
                ));
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS)
            {
                return 0;
            }
            return -1;
        }
        break;
        case 0:
        {
            ACE_DEBUG((LM_NOTICE, "[%D] ISGWIntf recv failed"
				",connection closed by foreign host"
                ",ret=%d,ip=%s\n"
                , ret, remote_addr_.get_host_addr()));
            return -1;
        }
        break;
        default: //接收成功
        recv_len_ += ret;
        ACE_DEBUG((LM_NOTICE, "[%D] ISGWIntf recv succ,ret=%d"
            ",recv_buf_=%s\n"
            , ret, recv_buf_
            ));

        //判断消息是否结束
        if (msg_len_ == 0) //没获取到消息包长度的情况
        {
            if (recv_len_ >= MSG_LEN_SIZE)
            {
                memcpy(&msg_len_, recv_buf_, MSG_LEN_SIZE);
                if(MSG_LEN_SIZE == 2)
                {
                    msg_len_ = ntohs(msg_len_);
                }
                else
                {
                    msg_len_ = ntohl(msg_len_);
                }
                
                if (msg_len_ > MAX_RECV_BUF_LEN || msg_len_ <= MSG_LEN_SIZE)
                {
                    ACE_DEBUG((LM_ERROR, "[%D] ISGWIntf recv failed,illegal msg"
                        ",msg_len_=%d,ip=%s\n"
                        , msg_len_, remote_addr_.get_host_addr()
                        ));
                    return -1;
                }
            }
            else
            {
                // not complete
                ACE_DEBUG((LM_TRACE, "[%D] ISGWIntf recv succ but uncompleted msg 1\n"));
                return 0;
            }
        }

        if (recv_len_ < (len_field_extra_size + msg_len_ ))
        {
            // not complete
            ACE_DEBUG((LM_TRACE, "[%D] ISGWIntf recv succ but uncompleted msg 2"
                ",recv_len_=%d,msg_len_=%d\n"
                , recv_len_, msg_len_
                ));
            return 0;
        }
        
    }

    //注释掉，同一个网络包有多个消息的情况下有可能会覆盖后面的消息头
    //recv_buf_[recv_len_] = '\0'; //最后一位置空表示字符串结束

    //已经获得完整的消息        
    unsigned int msg_len = msg_len_;
    unsigned int pend_len = recv_len_;//待处理的长度
    unsigned int proced_len = 0;	//已经处理的长度
    char* msg_buf = NULL;		//作为recv_buf_ 的游标指针
    int i = 0; //只是标记第几次循环 ? 

    //循环处理每一个完整的消息，不完整的需要等再接收完再处理
    while( pend_len >=  (len_field_extra_size + msg_len) )
    {
        ACE_DEBUG((LM_TRACE, "[%D] ISGWIntf in while,i=%d,pend_len=%d"
            ",proced_len=%d,msg_len=%d\n"
            , i, pend_len, proced_len, msg_len));
        
        //消息指针下移
        msg_buf = &recv_buf_[proced_len + len_field_extra_size];
        
        //解析当前消息长度指定的消息块，并进行处理
        if (process(msg_buf, get_handle(), get_seq(), msg_len) != 0)
        {
            //如果解析或者处理出错(后端无处理能力),整个消息一起丢弃,并导致断开连接    		
            return -1;
        }
        
        //已经处理的长度 增加
        proced_len += len_field_extra_size + msg_len;
        //待处理的长度减少
        pend_len -= len_field_extra_size + msg_len;
        
        
        //取下一个完整消息的长度
        //从proced_len 位置取出
        msg_len = 0;
        if (pend_len < MSG_LEN_SIZE)
        {
        	//如果剩余的长度不够四个字节直接跳出循环，继续接收消息才能处理        	
        	break;
        }
        memcpy(&msg_len , &recv_buf_[proced_len], MSG_LEN_SIZE);
        if(MSG_LEN_SIZE == 2)
        {
            msg_len = ntohs(msg_len);
        }
        else
        {
            msg_len = ntohl(msg_len);
        }
        ACE_DEBUG((LM_TRACE, "[%D] ISGWIntf end while,i=%d,MSG_LEN_SIZE=%d"
            ",pend_len=%d,proced_len=%d,msg_len=%d\n"
            , i, MSG_LEN_SIZE, pend_len, proced_len, msg_len
            ));
        
        // 如果msg_len 异常 则断开
        if (msg_len > MAX_RECV_BUF_LEN || msg_len <= MSG_LEN_SIZE)
        {
            ACE_DEBUG((LM_ERROR, "[%D] ISGWIntf recv failed,illegal msg"
                ",msg_len=%d,ip=%s\n"
                , msg_len, remote_addr_.get_host_addr()
                ));
            return -1;
        }
        
        i++;
    }    
    
    ACE_DEBUG((LM_DEBUG, "[%D] ISGWIntf out while,pend_len=%d,proced_len=%d,msg_len=%d\n"
		, pend_len, proced_len, msg_len
		));
    
    if ( pend_len == 0 )
    {
    	//正好处理完的情况
    	recv_len_ = 0; // reset the recv pos indicator
    	msg_len_  = 0;		
    }	
    else
    {	
    	//剩余的部分直接移动
    	ACE_DEBUG((LM_TRACE, "[%D] ISGWIntf memmove,proced_len=%d,pend_len=%d"
    	, proced_len, pend_len));
    	memmove(recv_buf_, &recv_buf_[proced_len], pend_len);    	
    	recv_len_ = pend_len;//接收到的消息长度赋值为待处理的长度
    	msg_len_ = msg_len;//下一个完整的消息长度等于while中最后一次取出的数据			
    }
    
    return 0;
}

#endif

int ISGWIntf::process(char* msg, int sock_fd, int sock_seq, int msg_len)
{
    ACE_DEBUG((LM_TRACE, "[%D] in ISGWIntf::process()\n"));
   
    PPMsg *req = NULL;
    int index = ISGW_Object_Que<PPMsg>::instance()->dequeue(req);
    if ( req == NULL )
    {
        ACE_DEBUG((LM_ERROR,
            "[%D] ISGWIntf dequeue msg failed,maybe system has no memory\n"
            ));
        return -1;
    }
    memset(req, 0x0, sizeof(PPMsg));
    req->cmd = 0;
    
    req->index = index;
    req->seq_no = msg_seq_++;
    req->sock_fd = sock_fd;
    req->protocol = PROT_TCP_IP;
    req->sock_ip = remote_addr_.get_ip_address();
    req->sock_port = remote_addr_.get_port_number();
    req->sock_seq = sock_seq;
    
    ::gettimeofday(&(req->tv_time), NULL);
    memcpy(req->msg, msg, msg_len);
    req->msg_len = msg_len;
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWIntf putq msg to ISGWMgrSvc"
        ",sock_fd=%u,protocol=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u,time=%u"
        ",msg_len=%d,msg=%s\n"
        , req->sock_fd
        , req->protocol
        , req->sock_ip
        , req->sock_port
        , req->sock_seq
        , req->seq_no
        , req->tv_time.tv_sec
        , req->msg_len
        , req->msg
        ));

    if (ISGWMgrSvc::instance()->putq(req) == -1)
    {
        //记录丢失的消息
        ACE_DEBUG((LM_ERROR, "[%D] ISGWIntf putq msg to ISGWMgrSvc failed"
            ",sock_fd=%u,protocol=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u,time=%u"
            ",msg_len=%d,msg=%s\n"
            , req->sock_fd
            , req->protocol
            , req->sock_ip
            , req->sock_port
            , req->sock_seq
            , req->seq_no
            , req->tv_time.tv_sec
            , req->msg_len
            , req->msg
            ));
        //入队失败回收消息，避免内存泄漏
        ISGW_Object_Que<PPMsg>::instance()->enqueue(req, req->index);
        // 统计计数 
        Stat::instance()->incre_stat(STAT_CODE_SVC_ENQUEUE);
        //入队失败可以返回失败 连接会断开 
        return -1;
    }

    ACE_DEBUG((LM_TRACE, "[%D] out ISGWIntf::process()\n"));	
    return 0;	
}

int ISGWIntf::handle_timeout(const ACE_Time_Value & tv, const void * arg)
{
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWIntf handle_timeout,lastrtime=%d,now=%d\n"
        , lastrtime_, ISGWAck::instance()->get_time()));
    if(ISGWAck::instance()->get_time() > lastrtime_ && 
        (ISGWAck::instance()->get_time()-lastrtime_) > MAX_IDLE_TIME_SEC)
    {
        ACE_DEBUG((LM_INFO, "[%D] ISGWIntf handle_timeout close with %s:%d"
            ",sock_seq:%d\n"
            , remote_addr_.get_host_addr()
            , remote_addr_.get_port_number()
            , AceSockHdrBase::get_seq()
            ));
        return -1; //this->handle_close ();  //the same
    }
    return 0;
}

int ISGWIntf::is_legal(char* msg)
{
    // 目前只判断是否有命令字
    if (strstr(msg, FIELD_NAME_CMD) == NULL)
    {
        return -1;
    }
    
    return 0;
}
int ISGWIntf::is_auth()
{
    int allow_flag = 0;//默认不做限制
    SysConf::instance()->get_conf_int("system", "allow_flag", &allow_flag);
	if (allow_flag == 0)
    {
        return 0;
    }
	
    char ip_list[1024] = {0};
    SysConf::instance()->get_conf_str("system", "allow_ip", ip_list, sizeof(ip_list));
    
    if (strstr(ip_list, remote_addr_.get_host_addr()) == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWIntf auth failed"
			",ip %s not allowed,close connection\n"
    		, remote_addr_.get_host_addr()
    		));
        return -1;
    }
    
    return 0;
}
