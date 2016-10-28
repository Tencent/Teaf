#include "isgw_uintf.h"
#include "isgw_mgr_svc.h"
#include "isgw_ack.h"
#include "stat.h"

ISGWUIntf* ISGWUIntf::instance_ = NULL;
ACE_UINT32 ISGWUIntf::msg_seq_ = 0;

ISGWUIntf* ISGWUIntf::instance()
{
    if (NULL == instance_)
    {
        instance_ = new ISGWUIntf();
    }
    return instance_;
}

ISGWUIntf::ISGWUIntf():ret_(0)
{
    ACE_DEBUG((LM_INFO, "[%D] construct ISGWUIntf succ\n"));
    memset(recv_buf_, 0, sizeof(recv_buf_));
}

ISGWUIntf::~ISGWUIntf()
{
    ACE_DEBUG((LM_INFO,"[%D] destruct ISGWUIntf succ"
                ",system errno=%d"
                ",errmsg=%s"
                "\n"
                , errno
                , strerror(errno)
                ));
}

int ISGWUIntf::open(const ACE_INET_Addr &svr_addr)
{    
    ret_ = dgram_.open(svr_addr);
    if (ret_ != 0 )
    {
        ACE_DEBUG((LM_ERROR,"[%D] ISGWUIntf open udp dgram socket failed"
                ",system errno=%d"
                ",errmsg=%s"
                "\n"
                , errno
                , strerror(errno)
                ));
        return ret_;
    }
    //register event handler
    if (ACE_Reactor::instance ()->register_handler(this, ACE_Event_Handler::READ_MASK) == -1)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWUIntf register failed\n"));
        return -1;
    }
    
    return 0;
}

ACE_HANDLE ISGWUIntf::get_handle (void) const
{
     return this->dgram_.get_handle ();
}


int ISGWUIntf::handle_close (ACE_HANDLE handle, ACE_Reactor_Mask mask)
{
    ACE_UNUSED_ARG (handle);
    ACE_UNUSED_ARG (mask);
    ACE_DEBUG ((LM_DEBUG,"[%D] (%P|%t) handle_close\n"));
    this->dgram_.close ();
    delete this;
    return 0;
}

int ISGWUIntf::handle_input(ACE_HANDLE)
{
    memset(recv_buf_, 0, sizeof(recv_buf_));

    //应该接收的最大消息长度，如果有消息头长度指定则等于消息头长度
    int max_recv_len = MAX_UDP_BUF_LEN; 
    
    ret_ = this->dgram_.recv((char*)recv_buf_ ,
                                max_recv_len, remote_addr_); //, &timeout
    if ( ret_ < 0 )
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWUIntf recv failed"
            ",ret=%d\n", ret_));
        //return -1; //UDP不能取消处理器事件检测
        return 0;
    }

    if(strstr(recv_buf_, MSG_SEPARATOR) == NULL)
    {
    	ACE_DEBUG((LM_ERROR, "[%D] ISGWUIntf recv failed, no end charctor\n"));
    	//return -1; //UDP不能取消处理器事件检测
    	return 0;
    }

    ACE_DEBUG((LM_TRACE, "[%D] ISGWUIntf recv succ"
    	",ret=%d,recv_buf_=%s\n", ret_, recv_buf_));
		
    //注释掉，同一个网络包有多个消息的情况下有可能会覆盖后面的消息头
	//recv_buf_[recv_len_] = '\0'; //最后一位置空表示字符串结束
        
    //简化处理，暂时不考虑多个消息组合在一起发送的，或者一次发送部分不完整的消息
    if (process(recv_buf_, get_handle(), msg_seq_) != 0)
    {
        //如果解析或者处理出错(后端无处理能力),整个消息一起丢弃,并导致断开连接    		
        //return -1; //UDP不能取消处理器事件检测
    }
    
    return 0;
}

int ISGWUIntf::process(char* msg, int sock_fd, int sock_seq)
{
    ACE_DEBUG((LM_TRACE, "[%D] in ISGWUIntf::process()\n"));
    
    //消息进行分段处理
    char req_msg[MAX_UDP_BUF_LEN];
    char* ptr = NULL;
    char* p = ACE_OS::strtok_r(msg, MSG_SEPARATOR, &ptr);
    while (p != NULL) 
    {
        memset(req_msg, 0x0, sizeof(req_msg));
        snprintf(req_msg, sizeof(req_msg)-1, "%s", p);

        PPMsg *req = NULL;
        int index = ISGW_Object_Que<PPMsg>::instance()->dequeue(req);
        if ( req == NULL )
        {
            ACE_DEBUG((LM_ERROR,
                "[%D] ISGWUIntf dequeue msg failed,maybe system has no memory\n"
                ));
            return -1;
        }
        memset(req, 0x0, sizeof(PPMsg));
        req->index = index;
        req->seq_no = msg_seq_++;
        req->sock_fd = sock_fd;
        req->protocol = PROT_UDP_IP;
        req->sock_ip = remote_addr_.get_ip_address();
        req->sock_port = remote_addr_.get_port_number();
        req->sock_seq = sock_seq;
        //req->cmd = 0;
        gettimeofday(&req->tv_time, NULL);
        snprintf(req->msg, sizeof(req->msg), "%s", req_msg);
        ACE_DEBUG((LM_NOTICE, "[%D] ISGWUIntf putq msg to ISGWMgrSvc"
        	",sock_fd=%u,protocol=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u,time=%u"
        	",msg=%s\n"
        	, req->sock_fd
        	, req->protocol
        	, req->sock_ip
        	, req->sock_port
        	, req->sock_seq
        	, req->seq_no
        	, req->tv_time.tv_sec
        	, req->msg
        	));
        if (ISGWMgrSvc::instance()->putq(req) == -1)
        {
            //记录丢失的消息
            ACE_DEBUG((LM_ERROR, "[%D] ISGWUIntf putq msg to ISGWMgrSvc failed"
            	",sock_fd=%u,protocol=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u,time=%u"
            	",msg=%s\n"
            	, req->sock_fd
            	, req->protocol
            	, req->sock_ip
            	, req->sock_port
            	, req->sock_seq
            	, req->seq_no
            	, req->tv_time.tv_sec
            	, req->msg
            	));
            //入队失败回收消息，避免内存泄漏
            ISGW_Object_Que<PPMsg>::instance()->enqueue(req, req->index);
            // 统计计数 
            Stat::instance()->incre_stat(STAT_CODE_SVC_ENQUEUE);
            
            return -1;
        }
        
        p = ACE_OS::strtok_r(NULL, MSG_SEPARATOR, &ptr);
    }

    ACE_DEBUG((LM_TRACE, "[%D] out ISGWUIntf::process()\n"));
    return 0;
}

int ISGWUIntf::is_legal(char* msg)
{
    if (strstr(msg, MSG_SEPARATOR) == NULL || strstr(msg, FIELD_NAME_CMD) == NULL)
	{
	    return -1;
	}
	
	return 0;
}

int ISGWUIntf::is_auth()
{
    int allow_flag = 0;//默认不做限制
    SysConf::instance()->get_conf_int("system", "udp_allow_flag", &allow_flag);
	if (allow_flag == 0)
    {
        return 0;
    }
    
    char ip_list[1024];
    memset(ip_list, 0x0, sizeof(ip_list));    
    SysConf::instance()->get_conf_str("system", "udp_allow_ip", ip_list, sizeof(ip_list));
    
    if (strstr(ip_list, remote_addr_.get_host_addr()) == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWUIntf auth failed"
			",ip %s not allowed,close connection\n"
    		, remote_addr_.get_host_addr()
    		));
        return -1;
    }
	
    return 0;
}

int ISGWUIntf::send_udp(char* ack_msg, int send_len, const ACE_INET_Addr& to_addr)
{   
    ACE_DEBUG((LM_TRACE,"[%D] ISGWUIntf start to send dmsg"
		", to addr=%s"
		", port=%d"
		", send len=%d"
		", msg=%s"
		"\n"
		, to_addr.get_host_addr()
		, to_addr.get_port_number()
		, send_len
		, ack_msg
		));

    int ret = dgram_.send(ack_msg, send_len, to_addr);
    if (ret == -1)
    {
        ACE_DEBUG((LM_ERROR,"[%D] ISGWUIntf send udp ack msg failed"
            ",system errno=%d"
            ",errmsg=%s"
            "\n"
            , errno
            , strerror(errno)
            ));
    }
    else
    {
        ACE_DEBUG((LM_TRACE,"[%D] ISGWUIntf send udp ack msg succ, ret=%d\n", ret));
    }
    
    return ret;
}

