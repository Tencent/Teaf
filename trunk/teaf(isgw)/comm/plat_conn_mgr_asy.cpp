#include "plat_conn_mgr_asy.h"
#include "isgw_ack.h"
#include "stat.h"
#include "asy_task.h"

ACE_Thread_Mutex PlatConnMgrAsy::conn_mgr_lock_;

//key: ip:fd   value: status 
//以ip 为key 的时候 同一个ip上就不能支持多个连接了 也就是不能支持转发到多个端口 
typedef map<string, int> CONN_STATUS_MAP;
static CONN_STATUS_MAP conn_status_; //存放连接信息 

int set_conn_status(string ip, int status)
{
    if(status == 0) 
    {
        conn_status_.erase(ip);
        return 0;
    }
    
    //直接修改连接状态，此处不要加锁
    conn_status_[ip] = status;
    return 0;
}

int get_conn_status(string ip)
{
    return conn_status_[ip];
}

PlatConnMgrAsy::PlatConnMgrAsy()
{
    memset(section_, 0x0, sizeof(section_));
    memset(ip_, 0x0, sizeof(ip_));    
    ip_num_ = IP_NUM_DEF; // 默认每组2个服务器 ip 
    port_ = 0;    
    time_out_.set(SOCKET_TIME_OUT/1000, (SOCKET_TIME_OUT%1000)*1000);
    memset(&conn_info_, 0x0, sizeof(conn_info_));    
}

PlatConnMgrAsy::PlatConnMgrAsy(const char*host_ip, int port)
{
    port_ = port;
    time_out_.set(SOCKET_TIME_OUT/1000, (SOCKET_TIME_OUT%1000)*1000);
    memset(&conn_info_, 0x0, sizeof(conn_info_));
    
    ip_num_ = IP_NUM_MAX;    
    for(int i = 0; i < ip_num_; ++i)
    {
        snprintf(ip_[i], sizeof(ip_[i]), "%s", host_ip);
        init_conn(i);
    }
}

PlatConnMgrAsy::~PlatConnMgrAsy()
{
    for (int i=0; i<IP_NUM_MAX; i++)
    {
        fini_conn(i);
    }
}

int PlatConnMgrAsy::init(const char *section)
{
    ACE_Guard<ACE_Thread_Mutex> guard(conn_mgr_lock_); 
    if (section != NULL)
    {
        strncpy(section_, section, sizeof(section_));
    }
    
    if ((SysConf::instance()->get_conf_int(section_, "ip_num", &ip_num_) != 0)
        || (ip_num_ > IP_NUM_MAX))
    {
        ip_num_ = IP_NUM_MAX;
    }
    
    int time_out = SOCKET_TIME_OUT;
    if (SysConf::instance()->get_conf_int(section_, "time_out", &time_out) == 0)
    {
        time_out_.set(time_out/1000, (time_out%1000)*1000);
    }
    
    //获取 ip 列表 
    char ip_str[16];
    for(int i=0; i<ip_num_; i++)
    {
        memset(ip_str, 0x0, sizeof(ip_str));
        snprintf(ip_str, sizeof(ip_str), "ip_%d", i);
        
        if (SysConf::instance()
            ->get_conf_str(section_, ip_str, ip_[i], sizeof(ip_[i])) != 0)
        {
            ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy get config failed"
                ",section=%s,ip_%d\n", section_, i));
            ip_num_ = i; //实际成功获取到的 ip 数量 
            break;
        }
    }
    
    if (ip_num_ == 0) 
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy init failed"
            ",section=%s,ip_num=%d\n", section_, ip_num_));
        ip_num_ = IP_NUM_DEF; //设回为默认值 
        return -1;
    }
    
    if (SysConf::instance()->get_conf_int(section_, "port", &port_) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy get port config failed"
            ",section=%s\n", section_)); 
        return -1;
    }
    
    ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrAsy start to init conn,ip_num=%d,section=%s,lock=0x%x\n"
            , ip_num_, section_, &(conn_mgr_lock_.lock()) ));
    for(int i=0; i<ip_num_; i++)
    {
        if (init_conn(i) !=0)
        {
            //某个连接不上 不退出程序 
            //return -1;
        }
    }

    //初始化一下 stat 保证不使用框架的模块使用也正常
    Stat::instance()->init("");
    return 0;
}

// ip port 不指定时 就使用内部的 ，指定时则替换内部的 
int PlatConnMgrAsy::init_conn(int ip_idx, const char *ip, int port)
{
    if (ip_idx<0 || ip_idx >=IP_NUM_MAX)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy init conn failed,para is invalid"
            ",ip_idx=%d\n", ip_idx));
        Stat::instance()->incre_stat(STAT_CODE_CONN_FAIL);
        return -1;
    }
    
    // 保证数据初始化的一致性 
    ACE_Guard<ACE_Thread_Mutex> guard(conn_lock_[ip_idx]);

    //这里需要再判断一次,如果已经建立好了连接则返回
    if (0 == is_estab(ip_idx))
    {
        ACE_DEBUG((LM_DEBUG,
            "[%D] PlatConnMgrAsy init_conn,intf is already connected"
            ",ip_idx=%d\n"
            , ip_idx
            ));
        return 0;
    }
    
    if (ip != NULL)
    {
        strncpy(ip_[ip_idx], ip, sizeof(ip_[ip_idx]));
    }
    if (port != 0)
    {
        port_ = port;
    }
	
    ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrAsy start to init conn"
        ",ip=%s,port=%d,ip_idx=%d,lock=0x%x\n"
        , ip_[ip_idx], port_, ip_idx, &(conn_lock_[ip_idx].lock()) ));

    // 没有配置的 ip 和端口 
    if (strlen(ip_[ip_idx]) == 0 || port_ == 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy init conn failed"
            ",ip=%s,port=%d,ip_idx=%d\n"
            , ip_[ip_idx], port_, ip_idx));
        Stat::instance()->incre_stat(STAT_CODE_CONN_FAIL);
        return -2; //配置不正常 
    }
    
    ACE_INET_Addr svr_addr(port_, ip_[ip_idx]);
    ISGW_CONNECTOR connector;
    conn_info_[ip_idx].intf = new ISGWCIntf();
    ACE_Synch_Options opt(2, time_out_);
    if (connector.connect(conn_info_[ip_idx].intf, svr_addr, opt) != 0) //, my_option
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy init conn failed"
            ",ip=%s,port=%d,ip_idx=%d\n"
            , ip_[ip_idx], port_, ip_idx
            ));
        //fini_conn(ip_idx);
        Stat::instance()->incre_stat(STAT_CODE_CONN_FAIL);
        return -1;
    }

    //保存连接信息 
    conn_info_[ip_idx].sock_fd = conn_info_[ip_idx].intf->get_handle();
    conn_info_[ip_idx].sock_seq = conn_info_[ip_idx].intf->get_seq();    
    ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrAsy init conn succ"
        ",ip=%s,port=%d,ip_idx=%d,sock_fd=%d,sock_seq=%d,cnaddr=%@\n"
        , ip_[ip_idx]
        , port_
        , ip_idx
        , conn_info_[ip_idx].sock_fd
        , conn_info_[ip_idx].sock_seq
        , &conn_info_
        ));
    ostringstream os;
    os<<ip_[ip_idx]<<":"<<conn_info_[ip_idx].sock_fd;
    set_conn_status(os.str(), 1);
    return 0;
}

int PlatConnMgrAsy::send(const void * buf, int len, const unsigned int uin)
{
    int ip_idx = get_ip_idx(uin);
    if (is_estab(ip_idx) != 0)
    {
        ACE_DEBUG((LM_ERROR,
            "[%D] PlatConnMgrAsy send failed"
            ",intf is disconn"
            ",ip_idx=%d\n"
            , ip_idx
            ));
        if (init_conn(ip_idx) !=0)
        {
            return -1;
        }
    }

    PPMsg *req = NULL; // 这里实际是向后台的请求消息 req
    int index = ISGW_Object_Que<PPMsg>::instance()->dequeue(req);
    if ( req == NULL )
    {
        ACE_DEBUG((LM_ERROR,
            "[%D] PlatConnMgrAsy dequeue msg failed"
            ",maybe system has no memory\n"
            ));
        return -1;
    }
    memset(req, 0x0, sizeof(PPMsg));
    req->index = index;

    req->sock_fd = conn_info_[ip_idx].sock_fd;
    req->protocol = PROT_TCP_IP;
    req->sock_seq = conn_info_[ip_idx].sock_seq;
    gettimeofday(&(req->tv_time), NULL);
    req->time = ISGWAck::instance()->get_time();
	req->msg_len = len;
	memcpy(req->msg, buf, req->msg_len);
    ACE_DEBUG((LM_NOTICE, "[%D] PlatConnMgrAsy send msg to ISGWAck"
        ",sock_fd=%u,protocol=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u,time=%u"
        ",len=%d,msg=%s\n"
        , req->sock_fd
        , req->protocol
        , req->sock_ip
        , req->sock_port
        , req->sock_seq
        , req->seq_no
        , req->time
        , req->msg_len
        , req->msg
        ));
    
    //放入回送消息管理器 负责把消息送到连接对端 (这里指跟后端的连接)
    ISGWAck::instance()->putq(req);
    return 0;
}

int PlatConnMgrAsy::send(const void * buf, int len, ASYRMsg & rmsg, const unsigned int uin)
{
	ASYTask::instance()->insert(rmsg);
	return send(buf, len, uin);
}

int PlatConnMgrAsy::get_ip_idx(const unsigned int uin)
{
    if (uin == 0)
    {
        return rand()%ip_num_;
    }
    
    return uin%ip_num_;
}

int PlatConnMgrAsy::is_estab(int ip_idx)
{
    //ISGWCIntf 里面会清理连接状态，这里只需要判断即可
    ostringstream os;
    os<<ip_[ip_idx]<<":"<<conn_info_[ip_idx].sock_fd;
    if (get_conn_status(os.str()) != 1)
    {
        ACE_DEBUG((LM_ERROR,"[%D] PlatConnMgrAsy check estab failed"
            ",ip=%s,port=%d\n", ip_[ip_idx], port_));
        return -1;
    }
    
    return 0;
}

int PlatConnMgrAsy::fini_conn(int ip_idx)
{
    ostringstream os;
    os<<ip_[ip_idx]<<":"<<conn_info_[ip_idx].sock_fd;
    set_conn_status(os.str(), 0);
    conn_info_[ip_idx].sock_fd = 0;
    conn_info_[ip_idx].sock_seq = 0;
    return 0;
}

