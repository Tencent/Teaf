#include "ace_sock_hdr_base.h"
#include "stat.h"
#include "pp_prot.h"
#include "isgw_oper_base.h"

unsigned int AceSockHdrBase::total_seq_ = 0;

AceSockHdrBase::AceSockHdrBase()
{
    ACE_DEBUG((LM_TRACE, "[%D] AceSockHdrBase start to construct\n"));
    time_null_ = NULL;
    time_zero_.set(0,0);
    sock_seq_ = 0;
}

AceSockHdrBase::~AceSockHdrBase()
{
    ACE_DEBUG(( LM_TRACE, "[%D] AceSockHdrBase start to destruct"
        //",remote host name=%s"
        ",addr=%s"
        ",fd=%d"
        ",errno=%d"
        ",errmsg=%s"
        "\n"
        //, remote_addr_.get_host_name()        //需要连接nscd服务,影响系能
        , remote_addr_.get_host_addr()
        , get_handle()
        , errno
        , strerror(errno)
        ));
    //析构时主动关闭连接 add by xwfang 2010-01-22
    //this->peer().close();
}

int AceSockHdrBase::open(void* p)
{
    if (super::open(p) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] AceSockHdrBase super::open failed\n"));
        return -1;
    }
    
    // 目前如果多线程使用这个类产生的序列号 sock_seq_ 是有可能重复的
    sock_seq_ = total_seq_++;
    
    this->peer().get_remote_addr(remote_addr_);
    ACE_DEBUG((LM_DEBUG, "[%D] AceSockHdrBase conn with %s:%d"
        ",sock_seq:%d\n"
        , remote_addr_.get_host_addr()
        , remote_addr_.get_port_number()
        , get_seq()));

    this->peer().enable(ACE_NONBLOCK); //显式设置成非阻塞模式 
    //将新建连接的发送和接受缓存区设置为64K
    int buf_size = 2*MAX_INNER_MSG_LEN;
    this->peer().set_option(SOL_SOCKET, SO_RCVBUF, (void*)&buf_size, sizeof(int));
    this->peer().set_option(SOL_SOCKET, SO_SNDBUF, (void*)&buf_size, sizeof(int));

    return 0;
}

int AceSockHdrBase::get_seq()
{
	return sock_seq_;
}

int AceSockHdrBase::handle_close (ACE_HANDLE fd, ACE_Reactor_Mask mask)
{
    ACE_DEBUG((LM_NOTICE, "[%D] AceSockHdrBase handle_close\n"));
    IsgwOperBase::instance()->handle_close(get_handle());
    super::handle_close(fd, mask);
    
    return 0;
}


int AceSockHdrBase::handle_output(ACE_HANDLE /*fd = ACE_INVALID_HANDLE*/)
{
    ACE_DEBUG((LM_NOTICE, "[%D] AceSockHdrBase handle_output\n"));
    return send();
}

int AceSockHdrBase::send(ACE_Message_Block* ack_msg /* = NULL */)
{
    if (ack_msg == NULL) // 内部事件触发时用 
    {
        if (this->msg_queue()->is_empty())
        {
            ACE_Reactor::instance()->cancel_wakeup(this, WRITE_MASK);
            return 0;
        }
        
        this->getq(ack_msg);
    }
    
    while (ack_msg)
    {
        ACE_DEBUG((LM_NOTICE, "[%D] AceSockHdrBase start to send ack_msg=0x%08x\n", ack_msg));
        //发送消息时确保使用非阻塞模式 timeout 用 NULL by xwfang 2010-01-22
        int ret = this->peer().send(ack_msg->rd_ptr(), ack_msg->length(), time_null_); 
        switch (ret)
        {
            case 0:
                // peer closed
                ACE_DEBUG((LM_ERROR, "[%D] AceSockHdrBase send failed"
                    ",connection closed by foreign host"
                    ",ret=%d"
                    ",ip=%s"
                    "\n"
                    , ret
                    , remote_addr_.get_host_addr()
                    ));
                ack_msg->release();
                ack_msg = NULL;
                Stat::instance()->incre_stat(STAT_CODE_ACK_DISCONN);
                return -1;
            case -1:
                if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS)
                {
                    ACE_DEBUG((LM_ERROR, "[%D] AceSockHdrBase send failed,ret=%d"
                        ",errno=%d,errmsg=%s,ip=%s\n"
                        , ret
                        , errno
                        , strerror(errno)
                        , remote_addr_.get_host_addr()
                        ));
				    
                    ack_msg->release();
                    ack_msg = NULL;
                    Stat::instance()->incre_stat(STAT_CODE_ACK_BLOCK);
                    return 0;
                }
                else
                {
                    // abnormal error
                    ACE_DEBUG((LM_ERROR, "[%D] AceSockHdrBase send failed,ret=%d"
                        ",errno=%d,errmsg=%s,ip=%s\n"
                        , ret
                        , errno
                        , strerror(errno)
                        , remote_addr_.get_host_addr()
                        ));
                    ack_msg->release();
                    ack_msg = NULL;
                    Stat::instance()->incre_stat(STAT_CODE_ACK_ABNOR);
                    return -1;
                }
            default:
                // normal case
                ack_msg->rd_ptr(ret);
                Stat::instance()->incre_stat(STAT_CODE_ACK_UNCOMP);
                break;
        }
        
        if (ack_msg->length() != 0)
        {
            ungetq(ack_msg);
            ACE_Reactor::instance()->schedule_wakeup(this, WRITE_MASK);
            break;
        }
        else
        {
            ACE_DEBUG((LM_NOTICE, "[%D] AceSockHdrBase send ack_msg=0x%08x succ\n", ack_msg));
            //正常发送完成，释放资源
            ack_msg->release();
            //delete ack_msg;
            ack_msg = NULL;
            
            if (this->msg_queue()->is_empty())
            {
                ACE_Reactor::instance()->cancel_wakeup(this, WRITE_MASK);
                break;
            }
            
            this->getq(ack_msg);
        }
    	
    }
    
    return 0;
}

int AceSockHdrBase::send_n(char* ack_msg, int len)
{
    ACE_DEBUG((LM_NOTICE, "[%D] AceSockHdrBase start to send_n ack_msg=0x%08x\n", ack_msg));
    //发送消息时确保使用非阻塞模式  如果发送的包过大(M级别) time_zero_ 应该要调大 不然会有问题
    int ret = this->peer().send_n(ack_msg, len, &time_zero_); 
    switch (ret)
    {
        case 0:
            // peer closed
            ACE_DEBUG((LM_ERROR, "[%D] AceSockHdrBase send_n failed"
                ",connection closed by foreign host"
                ",ret=%d\n"
                , ret));
            Stat::instance()->incre_stat(STAT_CODE_ACK_DISCONN);
            return -1;
        case -1:
            if (errno == EWOULDBLOCK || errno == EAGAIN 
                || errno == EINPROGRESS|| ETIME == errno )
            {
                ACE_DEBUG((LM_ERROR, "[%D] AceSockHdrBase send_n failed,ret=%d"
                    ",errno=%d,errmsg=%s,ip=%s\n"
                    , ret
                    , errno
                    , strerror(errno)
                    , remote_addr_.get_host_addr()
                    ));
                Stat::instance()->incre_stat(STAT_CODE_ACK_BLOCK);
                return -1;
            }
            else
            {
                // abnormal error
                ACE_DEBUG((LM_ERROR, "[%D] AceSockHdrBase send_n failed,ret=%d"
                    ",errno=%d,errmsg=%s,ip=%s\n"
                    , ret
                    , errno
                    , strerror(errno)
                    , remote_addr_.get_host_addr()
                    ));
                Stat::instance()->incre_stat(STAT_CODE_ACK_ABNOR);
                return -1;
            }
        default:
            if (ret != len)
            {
                ACE_DEBUG((LM_ERROR, "[%D] AceSockHdrBase send_n failed"
                    ",len=%d"
                    ",ret=%d"
                    ",ip=%s"
                    "\n"
                    , len
                    , ret
                    , remote_addr_.get_host_addr()
                    ));
                Stat::instance()->incre_stat(STAT_CODE_ACK_UNCOMP);
                return -1;
            }
            break;
    }

    ACE_DEBUG((LM_NOTICE, "[%D] AceSockHdrBase send_n ack_msg=0x%08x succ\n", ack_msg));
    return 0;
}

