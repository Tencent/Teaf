#include "isgw_cintf.h"
#include "stat.h"
#include "isgw_ack.h"
//从ack->msg 解析出消息的key信息，一般为sockfd sock_seq msg_seq
extern int isgw_cintf_parse(PPMsg *ack);
extern int set_conn_status(string , int);

int ISGWCIntf::msg_seq_ = 0;
ISGWC_MSGQ ISGWCIntf::queue_(MSG_QUE_SIZE_C, MSG_QUE_SIZE_C); //存放该连接上接收到的消息 
ACE_Time_Value ISGWCIntf::zero_(0,0);

// 需要定义的解析函数 (缺省)
int isgw_cintf_parse_deft(PPMsg * ack)
{
    QModeMsg qmode_ack(ack->msg);
    //如果需要直接返回给客户端可以把此字段设置为1
    ack->rflag = atoi((*(qmode_ack.get_map()))["_rflag"].c_str());
    ack->sock_fd = strtoul((*(qmode_ack.get_map()))["_sockfd"].c_str(), NULL, 10);
    ack->protocol = strtoul((*(qmode_ack.get_map()))["_prot"].c_str(), NULL, 10);
    ack->sock_seq = strtoul((*(qmode_ack.get_map()))["_sock_seq"].c_str(), NULL, 10);
    ack->seq_no = strtoul((*(qmode_ack.get_map()))["_msg_seq"].c_str(), NULL, 10);
    ack->cmd = qmode_ack.get_cmd();
    ack->time = strtoul((*(qmode_ack.get_map()))["_time"].c_str(), NULL, 10);
    return 0;
}

int ISGWCIntf::init()
{
    //设置消息队列大小 
    int quesize = MSG_QUE_SIZE_C;	
    SysConf::instance()->get_conf_int("isgw_cintf", "quesize", &quesize); 
    ACE_DEBUG((LM_INFO, "[%D] ISGWCIntf set quesize=%d(byte)\n", quesize));
    queue_.open(quesize, quesize, NULL);
    
    return 0;
}

ISGWCIntf::ISGWCIntf() : recv_len_(0), msg_len_(0)
{
    memset(recv_buf_, 0x0, sizeof(recv_buf_));
}

ISGWCIntf::~ISGWCIntf()
{
    
}

int ISGWCIntf::open(void* p)
{
    if (AceSockHdrBase::open(p) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWCIntf open failed,ip=%s\n"
            , remote_addr_.get_host_addr()));
        return -1;
    }
    
    //ip权限检验暂时取消
    if (is_auth() != 0)
    {
        return -1;
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWCIntf open succ,p=%@\n", p));
    
    return 0;
}

#ifndef MSG_LEN_SIZE_C 
int ISGWCIntf::handle_input(ACE_HANDLE /*fd = ACE_INVALID_HANDLE*/)
{    
    //接收消息
    int ret = this->peer().recv((char*)recv_buf_ + recv_len_,
                                MAX_RECV_BUF_LEN_C - recv_len_); //, &timeout
    switch(ret)
    {
        case -1:
        {
            ACE_DEBUG((LM_ERROR, "[%D] ISGWCIntf recv failed"
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
            ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf recv failed"
                ",connection closed by foreign host"
				",ret=%d,ip=%s\n", ret, remote_addr_.get_host_addr()));
            return -1;
        }
        break;
        default: //接收成功
        recv_len_ += ret;
        ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf recv succ,ret=%d,recv_buf_=%s\n"
            , ret, recv_buf_));

        //判断消息是否结束
        if (strstr(recv_buf_, MSG_SEPARATOR) == NULL) //未结束，继续接收
        {
            if ( recv_len_ < MAX_RECV_BUF_LEN_C )
            {
                return 0;
            }
            else
            {
                ACE_DEBUG((LM_ERROR, "[%D] ISGWCIntf recv failed,illegal msg"
                    ",reach max buff len and have no msg end,discard it,ip=%s\n"
                    , remote_addr_.get_host_addr()));
                return -1; //消息非法
            }
        }        
    }

    //注释掉，同一个网络包有多个消息的情况下有可能会覆盖后面的消息头
    //recv_buf_[recv_len_] = '\0';


    //已经获得完整的消息
    
    //判断消息是否非法
    if (is_legal(recv_buf_) != 0)
    {
    	ACE_DEBUG((LM_ERROR, "[%D] ISGWCIntf recv failed,illegal msg,discard it"
            ",ip=%s\n", remote_addr_.get_host_addr()));
    	recv_len_ = 0; // reset the recv pos indicator 
    	memset(recv_buf_, 0x0, sizeof(recv_buf_));
    	return 0;
    }

    //简化处理，暂时不考虑多个消息组合在一起发送的，或者一次发送部分不完整的消息
    char *msg_start = recv_buf_;
    char *msg_end = NULL;
    int proced_len = 0; //已经处理的长度
    int pend_len = recv_len_; //剩余的消息长度
    
    while ((msg_end = strstr(msg_start, MSG_SEPARATOR)) != NULL)
    {
        msg_len_ = msg_end + strlen(MSG_SEPARATOR)-msg_start;  
        if (process(msg_start, get_handle(), get_seq(), msg_len_) != 0)
        {
            //如果解析或者处理出错(后端无处理能力),整个消息一起丢弃,并导致断开连接    		
            return -1;
        }
        
        proced_len += msg_len_;
        pend_len -= msg_len_;
        msg_start = msg_end + strlen(MSG_SEPARATOR);  
        
        ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf process msg msg_len=%d"
			", proced_len=%d, pend_len=%d"
			"\n"
			, msg_len_
			, proced_len
			, pend_len
			));
    }
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWCIntf out while,pend_len=%d,proced_len=%d\n"
		, pend_len, proced_len
		));
    
    //移动剩下的数据
    memmove(recv_buf_, msg_start, pend_len);
    memset(recv_buf_+pend_len, 0x0, sizeof(recv_buf_)-pend_len);    
    recv_len_ = pend_len; // reset the recv pos indicator    
    
    return 0;
}

#else

int ISGWCIntf::handle_input(ACE_HANDLE /*fd = ACE_INVALID_HANDLE*/)
{
    //接收消息
    int ret = this->peer().recv((char*)recv_buf_ + recv_len_,
                                MAX_RECV_BUF_LEN_C - recv_len_); //, &timeout
    switch(ret)
    {
        case -1:
        {            
            ACE_DEBUG((LM_ERROR, "[%D] ISGWCIntf recv failed"
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
            ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf recv failed"
				",connection closed by foreign host"
                ",ret=%d,ip=%s\n"
                , ret, remote_addr_.get_host_addr()));
            return -1;
        }
        break;
        default: //接收成功
        recv_len_ += ret;
        ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf recv succ,ret=%d"
            ",recv_buf_=%s\n"
            , ret, recv_buf_
            ));

        //判断消息是否结束
        if (msg_len_ == 0)
        {
            if (recv_len_ >= MSG_LEN_SIZE_C)
            {
                memcpy(&msg_len_, recv_buf_, MSG_LEN_SIZE_C);
                msg_len_ = ntohl(msg_len_);
                if (msg_len_ > MAX_RECV_BUF_LEN_C)
                {
                    ACE_DEBUG((LM_ERROR, "[%D] ISGWCIntf recv failed,illegal msg"
                        ",msg_len_=%d>MAX_RECV_BUF_LEN_C=%d,ip=%s\n"
                        , msg_len_, MAX_RECV_BUF_LEN_C, remote_addr_.get_host_addr()
                        ));
                    return -1;
                }
            }
            else
            {
                // not complete
                ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf recv succ but uncompleted msg 1\n"));
                return 0;
            }
        }
    
        if (recv_len_ - MSG_LEN_SIZE_C < msg_len_)
        {
            // not complete
            ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf recv succ but uncompleted msg 2"
                ",recv_len_=%d,msg_len_=%d\n"
                , recv_len_, msg_len_
                ));
            return 0;
        }
        
    }

    //注释掉，同一个网络包有多个消息的情况下有可能会覆盖后面的消息头
    //recv_buf_[recv_len_] = '\0'; 

    //已经获得完整的消息        
    unsigned int msg_len = msg_len_;
	
    unsigned int pend_len = recv_len_;//待处理的长度
    unsigned int proced_len = 0;	//已经处理的长度
    char* msg_buf = NULL;		//作为recv_buf_ 的游标指针
    int i = 0;

    //循环处理每一个完整的消息
    while( pend_len >=  (MSG_LEN_SIZE_C + msg_len) )
    {			
        ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf in while,i=%d,pend_len=%d"
            ",proced_len=%d,msg_len=%d\n"
            , i++, pend_len, proced_len, msg_len));
        
        //消息指针下移
        msg_buf = &recv_buf_[proced_len + MSG_LEN_SIZE_C];
        
        //解析当前消息长度指定的消息块，并进行处理
        if (process(msg_buf, get_handle(), get_seq(), msg_len) != 0)
        {
            //如果解析或者处理出错(后端无处理能力),整个消息一起丢弃,并导致断开连接    		
            return -1;
        }
        
        //已经处理的长度 增加
        proced_len += MSG_LEN_SIZE_C+msg_len;
        //待处理的长度减少
        pend_len -= MSG_LEN_SIZE_C+msg_len;    	
        
        if (pend_len < MSG_LEN_SIZE_C)
        {
        	//如果剩余的长度不够四个字节直接退出
        	msg_len = 0;
        	break;
        }
        
        //取下一个完整消息的长度
        //从proced_len 位置取出四个字节的数据
        memcpy(&msg_len, &recv_buf_[proced_len], MSG_LEN_SIZE_C);
        msg_len = ntohl(msg_len);
        ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf end while,i=%d,MSG_LEN_SIZE_C=%d"
            ",pend_len=%d,proced_len=%d,msg_len=%d\n"
            , i, MSG_LEN_SIZE_C, pend_len, proced_len, msg_len
            ));
        
    }    
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWCIntf out while,pend_len=%d,proced_len=%d,msg_len=%d\n"
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
    	ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf memmove,proced_len=%d,pend_len=%d"
    	, proced_len, pend_len));    	
    	memmove(recv_buf_, &recv_buf_[proced_len], pend_len);    	
    	recv_len_ = pend_len;//接收到的消息长度赋值为待处理的长度
    	msg_len_ = msg_len;//下一个完整的消息长度等于while中最后一次取出的数据			
    }
    
    return 0;
}

#endif

int ISGWCIntf::process(char* msg, int sock_fd, int sock_seq, int msg_len)
{
    ACE_DEBUG((LM_TRACE, "[%D] in ISGWCIntf::process()\n"));
    //消息进行分段处理
    char tmp_msg[MAX_RECV_BUF_LEN_C] = {0};
    memcpy(tmp_msg, msg, msg_len);
    
    char* ptr = NULL;
    char* p = ACE_OS::strtok_r(tmp_msg, MSG_SEPARATOR, &ptr);
    while (p != NULL) 
    {
        PPMsg *ack = NULL;
        int index = ISGW_Object_Que<PPMsg>::instance()->dequeue(ack);
        if ( ack == NULL )
        {
            ACE_DEBUG((LM_ERROR,
                "[%D] ISGWCIntf dequeue msg failed,maybe system has no memory\n"
                ));
            return -1;
        }
        memset(ack, 0x0, sizeof(PPMsg));
        ack->index = index;
        snprintf(ack->msg, sizeof(ack->msg)-1, "%s", p);
        // 获取到原始请求的前端连接信息 
        ack->sock_ip = remote_addr_.get_ip_address();
        ack->sock_port = remote_addr_.get_port_number();
        ::gettimeofday(&(ack->tv_time), NULL);
#ifdef _ISGW_CINTF_PARSE_ 
        isgw_cintf_parse(ack);
#else
	isgw_cintf_parse_deft(ack);
#endif
        ACE_DEBUG((LM_NOTICE, "[%D] ISGWCIntf process msg"
            ",rflag=%d,sock_fd=%u,protocol=%u"
            ",ip=%u,port=%u,sock_seq=%u,seq_no=%u,time=%u"
            ",msg=%s\n"
            , ack->rflag
            , ack->sock_fd
            , ack->protocol
            , ack->sock_ip
            , ack->sock_port
            , ack->sock_seq
            , ack->seq_no
            , ack->time
            , ack->msg
            ));
        
        //根据 rflag 判断 是直接返回客户端 还是 放到自身的消息队列中 
        if (ack->rflag != 0)
        {
            strncat(ack->msg, MSG_SEPARATOR, strlen(MSG_SEPARATOR));//带上协议结束符 
            ISGWAck::instance()->putq(ack);
        }
        else
        {
            int ret = queue_.enqueue(ack, &zero_);
            ACE_DEBUG((LM_NOTICE, "[%D] ISGWCIntf process enqueue msg,ret=%d\n", ret));
            if (ret == -1)
            {
                //记录丢失的消息
                ACE_DEBUG((LM_ERROR, "[%D] ISGWCIntf process enqueue msg failed"
                    ",rflag=%d,sock_fd=%u,protocol=%u"
                    ",ip=%u,port=%u,sock_seq=%u,seq_no=%u,time=%u"
                    ",msg=%s\n"
                    , ack->rflag
                    , ack->sock_fd
                    , ack->protocol
                    , ack->sock_ip
                    , ack->sock_port
                    , ack->sock_seq
                    , ack->seq_no
                    , ack->time
                    , ack->msg
                    ));
                // 入队失败回收消息，避免内存泄漏
                ISGW_Object_Que<PPMsg>::instance()->enqueue(ack, ack->index);
                // 统计计数 
                Stat::instance()->incre_stat(STAT_CODE_ISGWC_ENQUEUE);
                // 与后端的连接模块 此处不必要断开连接 
                //return -1;
            }
        }
                
        p = ACE_OS::strtok_r(NULL, MSG_SEPARATOR, &ptr);
    }
    
    ACE_DEBUG((LM_TRACE, "[%D] out ISGWCIntf::process()\n"));	
    return 0;	
}

int ISGWCIntf::handle_close(ACE_HANDLE fd, ACE_Reactor_Mask mask)
{
    //设置连接状态为不可用 
    //ostringstream os;
    //os<<this->get_handle()<<" "<<this->get_seq();    
    string ip = remote_addr_.get_host_addr();
    ACE_DEBUG((LM_INFO, "[%D] ISGWCIntf handle_close,ip=%s\n", ip.c_str()));
    set_conn_status(ip, 0);
    
    this->destroy();
    return 0;
}

int ISGWCIntf::is_legal(char* msg)
{
    if (strstr(msg, MSG_SEPARATOR) == NULL || strstr(msg, FIELD_NAME_CMD) == NULL)
    {
        return -1;
    }
	
    return 0;
}
int ISGWCIntf::is_auth()
{
    int allow_flag = 0;//默认不做限制
    SysConf::instance()->get_conf_int("system", "allow_flag", &allow_flag);
	if (allow_flag == 0)
    {
        return 0;
    }
	
    char ip_list[1024];
    memset(ip_list, 0x0, sizeof(ip_list));    
    SysConf::instance()->get_conf_str("system", "allow_ip", ip_list, sizeof(ip_list));
    
    if (strstr(ip_list, remote_addr_.get_host_addr()) == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWCIntf auth failed"
			",ip %s not allowed,close connection\n"
    		, remote_addr_.get_host_addr()
    		));
        return -1;
    }
    
    return 0;
}

int ISGWCIntf::recvq(PPMsg*& msg, ACE_Time_Value* time_out)
{
    int ret = 0;
    
    ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf before recvq\n" ));
    
    ret = queue_.dequeue(msg, time_out);
    if ( msg == NULL || ret == -1 )
    {
        ACE_DEBUG((LM_TRACE,
            "[%D] ISGWCIntf recvq,dequeue msg failed or recv a null msg\n"
            ));
        return ret;
    }
    ACE_DEBUG((LM_TRACE, "[%D] ISGWCIntf after recvq,msg count %d\n", ret));
    
    return ret;
}
