#include "plat_conn_mgr_ex.h"
#include "stat.h"
#include <fstream>

PlatConnMgrEx::PlatConnMgrEx()
{
    for(int i=0; i<IP_NUM_MAX; i++)
    {
        for(int j=0; j<POOL_CONN_MAX; j++)
        {
            conn_[i][j] = NULL;
    		conn_use_flag_[i][j] = 0;
        }
        memset(ip_[i], 0x0, sizeof(ip_[i]));
        fail_times_[i] = 0;
        last_fail_time_[i] = 0;
    }

    ip_num_ = IP_NUM_DEF;
    conn_nums_ = POOL_CONN_DEF;
    
    time_out_.set(SOCKET_TIME_OUT/1000, (SOCKET_TIME_OUT%1000)*1000);
    memset(section_, 0x0, sizeof(section_));	
    port_ = 0;

    use_strategy_ = DEF_USE_STRATEGY; //默认使用
    max_fail_times_ = DEF_MAX_FAIL_TIMES; //默认连续五次失败则等待一段时间重连
	recon_interval_ = DEF_RECON_INTERVAL; //默认10s重新连接 
	
}

PlatConnMgrEx::PlatConnMgrEx(const char*host_ip, int port, int conn_num)
{
    for(int i=0; i<IP_NUM_MAX; i++)
    {
        for(int j=0; j<POOL_CONN_MAX; j++)
        {
            conn_[i][j] = NULL;
            conn_use_flag_[i][j] = 0;            
        }
        memset(ip_[i], 0x0, sizeof(ip_[i]));
        fail_times_[i] = 0;
        last_fail_time_[i] = 0;
    }

    ip_num_ = 1;
    snprintf(ip_[0], sizeof(ip_[0]), "%s", host_ip);
    port_ = port;
    conn_nums_ = conn_num;
    time_out_.set(SOCKET_TIME_OUT/1000, (SOCKET_TIME_OUT%1000)*1000);

    use_strategy_ = DEF_USE_STRATEGY; //默认使用
    max_fail_times_ = DEF_MAX_FAIL_TIMES; //默认连续五次失败则等待一段时间重连
	recon_interval_ = DEF_RECON_INTERVAL; //默认10s重新连接 
}

PlatConnMgrEx::~PlatConnMgrEx()
{
    for(int i=0; i<IP_NUM_MAX; i++)
    {
        for(int j=0; j<POOL_CONN_MAX; j++)
        {
            if (conn_[i][j] != NULL)
            {
                conn_[i][j]->close();
                delete conn_[i][j];
                conn_[i][j] = NULL;
    			conn_use_flag_[i][j] = 0;
            }
        }
    }
    
}

int PlatConnMgrEx::init(const char *section, const std::vector<std::string> *ip_vec)
{
    ACE_Guard<ACE_Thread_Mutex> guard(conn_mgr_lock_); 
    if (section != NULL)
    {
        strncpy(section_, section, sizeof(section_));
    }

    // 默认使用用户指定的ip
    if(ip_vec != NULL && !ip_vec->empty())
    {
        ip_num_ = static_cast<int>(ip_vec->size());
        if(ip_num_ > IP_NUM_MAX) ip_num_ = IP_NUM_MAX;
        for(int i = 0;i < ip_num_;i++)
        {
            snprintf(ip_[i], sizeof(ip_[i]), "%s", (*ip_vec)[i].c_str());
        }
    }
    else /* 未指定ip则使用配置的ip */
    {
        if ((SysConf::instance()->get_conf_int(section_, "ip_num", &ip_num_) != 0)
    		|| (ip_num_ > IP_NUM_MAX))
    	{
    		ip_num_ = IP_NUM_DEF;
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
    			ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx get config failed"
    				",section=%s,ip_%d\n", section_, i));
    			ip_num_ = i; //实际成功获取到的 ip 数量 
    			break;
    		}
    	}
    }
    
	if (ip_num_ == 0) //一个ip都没有 
	{
		ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx init failed"
			",section=%s,ip_num=%d\n", section_, ip_num_));
		ip_num_ = IP_NUM_DEF; //设回为默认值 
		return -1;
	}
	
	if (SysConf::instance()->get_conf_int(section_, "conn_nums", &conn_nums_) != 0)
	{
		conn_nums_ = POOL_CONN_DEF;
	}
	else if(conn_nums_ > POOL_CONN_MAX)
	{
		conn_nums_ = POOL_CONN_MAX;
	}
	
	int time_out = SOCKET_TIME_OUT;
	if (SysConf::instance()->get_conf_int(section_, "time_out", &time_out) == 0)
	{
		time_out_.set(time_out/1000, (time_out%1000)*1000);
	}

	if (SysConf::instance()->get_conf_int(section_, "port", &port_) != 0)
	{
		ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx get port config failed"
			",section=%s\n", section_)); 
		return -1;
	}

	if (SysConf::instance()->get_conf_int(section_, "use_strategy", &use_strategy_) == 0)
	{
		//使用连接策略
		ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx get use_strategy config succ,use strategy"
			",section=%s\n", section_));
		SysConf::instance()->get_conf_int(section_, "max_fail_times", &max_fail_times_);
		SysConf::instance()->get_conf_int(section_, "recon_interval", &recon_interval_);        
	}

    ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrEx start to init conn"
        ",ip_num=%d"
        ",conn_nums=%d"
        ",lock=0x%x"
        "\n"
        , ip_num_
        , conn_nums_
        ,&(conn_mgr_lock_.lock())
        )); 
    for(int i=0; i<ip_num_; i++)
    {
        for(int j=0; j<conn_nums_; j++)
        {
            if (init_conn(j, i) !=0)
            {
                //某个连接不上 不退出程序 
                //return -1;
            }
        }
    }
    
    //初始化一下 stat 保证不使用框架的模块使用也正常
    Stat::instance()->init();
    return 0;
}

// ip port 不指定时 就使用内部的 ，指定时则替换内部的 
int PlatConnMgrEx::init_conn(int index, int ip_idx, const char *ip, int port)
{
    if (index<0 || index>=POOL_CONN_MAX || ip_idx<0 || ip_idx >=IP_NUM_MAX)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx init conn failed,para is invalid"
            ",section=%s,index=%d,ip_idx=%d\n", section_, index, ip_idx));
        Stat::instance()->incre_stat(STAT_CODE_CONN_FAIL);
        return -1;
    }
    
    if (ip != NULL)
    {
        strncpy(ip_[ip_idx], ip, sizeof(ip_[ip_idx]));
    }
    if (port != 0)
    {
        port_ = port;
    }
	
    if (conn_[ip_idx][index] != NULL)
    {
        fini(index, ip_idx);
    }

    ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrEx init conn"
        ",ip=%s"
        ",port=%d"
        ",index=%d"
        ",ip_idx=%d"
        "\n"
        , ip_[ip_idx]
        , port_
        , index
        , ip_idx
        ));

    // 没有配置的 ip 和端口 
    if (strlen(ip_[ip_idx]) == 0 || port_ == 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx init conn failed,invalid para"
            ",ip=%s,port=%d,index=%d,ip_idx=%d\n"
            , ip_[ip_idx], port_, index, ip_idx));
        Stat::instance()->incre_stat(STAT_CODE_CONN_FAIL);
        return -2; //配置不正常 
    }

    //主机连接不上，连接备机 
    ACE_INET_Addr svr_addr(port_, ip_[ip_idx]);
    ACE_SOCK_Connector connector;
    conn_[ip_idx][index] = new ACE_SOCK_Stream();    
    if (connector.connect(*conn_[ip_idx][index], svr_addr, &time_out_) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx init conn failed,conn failed"
            ",ip=%s,port=%d,index=%d,ip_idx=%d\n"
            , ip_[ip_idx], port_, index, ip_idx));
        fini(index, ip_idx);
        Stat::instance()->incre_stat(STAT_CODE_CONN_FAIL);
        return -1;
    }
    
    ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrEx init conn succ"
        ",ip=%s"
        ",port=%d"
        ",index=%d"
        ",ip_idx=%d"
        ",lock=0x%x"
        "\n"
        , ip_[ip_idx]
        , port_
        , index
        , ip_idx
        , &(conn_lock_[ip_idx][index].lock())
        ));
    // 连接成功实际连续失败次数要清 0  使用状态要清 0 
    fail_times_[ip_idx] = 0;
    conn_use_flag_[ip_idx][index] = 0;
    return 0;
}

int PlatConnMgrEx::send(const void * buf, int len, const unsigned int uin)
{
    int ip_idx = get_ip_idx(uin);
    int index = get_index(ip_idx, uin);
    ACE_SOCK_Stream* conn = get_conn(index, ip_idx);
    if (conn == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx get conn failed"
            ",can't get a useful conn"
            ",section=%s,index=%d,ip_idx=%d,uin=%u\n"
            , section_, index, ip_idx, uin
            ));
        return -1;
    }
    ACE_Guard<ACE_Thread_Mutex> guard(conn_lock_[ip_idx][index]);// 加锁需要放在获取的后面 
    if (conn_[ip_idx][index] == NULL) // 加锁后增加一次非法判断 
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send failed"
            ",conn is null"
            ",section=%s,index=%d,ip_idx=%d,uin=%u\n"
            , section_, index, ip_idx, uin
            ));
        return -1;
    }
    conn_use_flag_[ip_idx][index] = 1;

    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx start to send msg"
        ",index=%d,ip_idx=%d,uin=%u\n", index, ip_idx, uin));
    int ret = conn_[ip_idx][index]->send(buf, len, &time_out_);
    if (ret <= 0) //异常或者对端关闭
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send msg failed"
            ",section=%s,index=%d,ip_idx=%d,ret=%d,uin=%u,errno=%d\n"
            , section_, index, ip_idx, ret, uin, errno));
        //关闭连接
        fini(index, ip_idx);
        Stat::instance()->incre_stat(STAT_CODE_SEND_FAIL);
        return ret;
    }
    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx send msg succ"
        ",index=%d,ip_idx=%d,ret=%d,uin=%u\n", index, ip_idx, ret, uin));

    conn_use_flag_[ip_idx][index] = 0;//退出前清理使用状态
    return ret;
}

int PlatConnMgrEx::send(const void * buf, int len, const std::string& ip)
{
    int ip_idx = get_ip_idx(ip);
    int index = get_index(ip_idx);
    ACE_SOCK_Stream* conn = get_conn(index, ip_idx);
    if (conn == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx get conn failed"
            ",can't get a useful conn"
            ",section=%s,index=%d,ip_idx=%d,ip=%s\n"
            , section_, index, ip_idx, ip.c_str()
            ));
        return -1;
    }
    ACE_Guard<ACE_Thread_Mutex> guard(conn_lock_[ip_idx][index]);// 加锁需要放在获取的后面 
    if (conn_[ip_idx][index] == NULL) // 加锁后增加一次非法判断 
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send failed"
            ",conn is null"
            ",section=%s,index=%d,ip_idx=%d,ip=%s\n"
            , section_, index, ip_idx, ip.c_str()
            ));
        return -1;
    }
    conn_use_flag_[ip_idx][index] = 1;

    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx start to send msg"
        ",index=%d,ip_idx=%d,ip=%s\n", index, ip_idx, ip.c_str()));
    int ret = conn_[ip_idx][index]->send(buf, len, &time_out_);
    if (ret <= 0) //异常或者对端关闭
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send msg failed"
            ",section=%s,index=%d,ip_idx=%d,ret=%d,ip=%s,errno=%d\n"
            , section_, index, ip_idx, ret, ip.c_str(), errno));
        //关闭连接
        fini(index, ip_idx);
        Stat::instance()->incre_stat(STAT_CODE_SEND_FAIL);
        return ret;
    }
    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx send msg succ"
        ",index=%d,ip_idx=%d,ret=%d,ip=%s\n", index, ip_idx, ret, ip.c_str()));

    conn_use_flag_[ip_idx][index] = 0;//退出前清理使用状态
    return ret;
}


int PlatConnMgrEx::send_recv(const void * send_buf, int send_len
    , void * recv_buf, int recv_len, const unsigned int uin)
{
    int ip_idx = get_ip_idx(uin);
    int index = get_index(ip_idx, uin);
    ACE_SOCK_Stream* conn = get_conn(index, ip_idx);
    if (conn == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx get conn failed"
            ",can't get a useful conn"
            ",section=%s,index=%d,ip_idx=%d,uin=%u\n"
            , section_, index, ip_idx, uin
            ));
        return -1;
    }
    ACE_Guard<ACE_Thread_Mutex> guard(conn_lock_[ip_idx][index]);// 加锁需要放在获取的后面 
    if (conn_[ip_idx][index] == NULL) // 加锁后增加一次非法判断 
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send_recv failed"
            ",conn is null"
            ",section=%s,index=%d,ip_idx=%d,uin=%u\n"
            , section_, index, ip_idx, uin
            ));
        return -1;
    }
    conn_use_flag_[ip_idx][index] = 1;
    
    //清除原缓冲 add by xwfang 2010-03-02
    ACE_Time_Value zero;
    zero.set(0);
    int max_recv_len = recv_len;
    int tmp_ret = conn_[ip_idx][index]->recv(recv_buf, max_recv_len, &zero);//不等待直接返返回
    // 检测到连接关闭时, 重建连接
    if((tmp_ret < 0 && errno != ETIME)   // 连接上无数据的情况,会返回超时,不需重连 
        || tmp_ret == 0)                 // 连接已经被对端关闭, 处于close wait状态
    {
        ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrEx send_recv connection close detected,"
            "uin=%u,ip=%s,index=%d\n", uin, ip_[ip_idx], index));
        init_conn(index, ip_idx);
        if(conn_[ip_idx][index] == NULL)
        {
            ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send_recv reconnect failed"
                ",section=%s,index=%d,ip_idx=%d\n", section_, index, ip_idx));
            return -1;
        }
    }
    
    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx send_recv msg"
        ",index=%d,ip_idx=%d,uin=%u\n", index, ip_idx, uin));
    int ret = conn_[ip_idx][index]->send(send_buf, send_len, &time_out_);
    if (ret <= 0) //异常或者对端关闭
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send_recv send msg failed"
            ",section=%s,index=%d,ip_idx=%d,ret=%d,uin=%u,errno=%d\n"
            , section_, index, ip_idx, ret, uin, errno));
        //关闭连接
        fini(index, ip_idx);
        Stat::instance()->incre_stat(STAT_CODE_SEND_FAIL);
        return ret;
    }
	// 
    ret = conn_[ip_idx][index]->recv(recv_buf, recv_len, &time_out_);
    if (ret <= 0) //异常或者对端关闭
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send_recv recv msg failed"
            ",section=%s,index=%d,ip_idx=%d,ret=%d,uin=%u,errno=%d\n"
            , section_, index, ip_idx, ret, uin, errno));
        //关闭连接
        fini(index, ip_idx);
        Stat::instance()->incre_stat(STAT_CODE_RECV_FAIL);
        return ret;
    }

    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx send_recv msg succ"
        ",index=%d,ip_idx=%d,ret=%d,uin=%u\n", index, ip_idx, ret, uin));
        
    conn_use_flag_[ip_idx][index] = 0;//退出前清理使用状态
    return ret;
    
}

// 支持指定接收的结束符来接收数据 add by xwfang 2010-03-02
int PlatConnMgrEx::send_recv_ex(const void * send_buf, int send_len
    , void * recv_buf, int recv_len, const char * separator, const unsigned int uin)
{
    int ip_idx = get_ip_idx(uin); // ip 默认根据 uin 轮询 
    int index = get_index(ip_idx, uin); 
    ACE_SOCK_Stream* conn = get_conn(index, ip_idx);
    if (conn == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx get conn failed"
            ",can't get a useful conn"
            ",section=%s,index=%d,ip_idx=%d,uin=%u\n"
            , section_, index, ip_idx, uin
            ));
        return -1;
    }
    
    // 因为 ip_idx 可能被修改，加锁需要放在获取的后面 
    ACE_Guard<ACE_Thread_Mutex> guard(conn_lock_[ip_idx][index]); 
    // 加锁后增加一次非法判断 不然有可能被其他线程释放连接对象
    if (conn_[ip_idx][index] == NULL) 
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send_recv_ex failed"
            ",conn is null"
            ",section=%s,index=%d,ip_idx=%d,uin=%u\n"
            , section_, index, ip_idx, uin
            ));
        return -1;
    }
    conn_use_flag_[ip_idx][index] = 1;

    //清除原缓冲 add by xwfang 2010-03-02
    ACE_Time_Value zero;
    zero.set(0);
    int max_recv_len = recv_len;
    int tmp_ret = conn_[ip_idx][index]->recv(recv_buf, max_recv_len, &zero);//不等特直接返返回
    // 检测到连接关闭时, 重建连接
    // 在对面进程重启时, 有可能发生
    if((tmp_ret < 0 && errno != ETIME)   // 连接上无数据的情况,会返回超时,不需重连 
        || tmp_ret == 0)                 // 连接已经被对端关闭, 处于close wait状态
    {
        ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrEx send_recv_ex connection close detected,"
            "uin=%u,ip=%s,index=%d\n", uin, ip_[ip_idx], index));
        init_conn(index, ip_idx);
    	if(conn_[ip_idx][index] == NULL)
    	{
    		ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send_recv_ex reconnect failed"
                ",section=%s,index=%d,ip_idx=%d\n", section_, index, ip_idx));
    		return -1;
    	}
    }

    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx send_recv_ex msg"
        ",index=%d,ip_idx=%d,uin=%u\n", index, ip_idx, uin));
    int ret = conn_[ip_idx][index]->send_n(send_buf, send_len, &time_out_);
    if (ret <= 0) //异常或者对端关闭
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send_recv_ex send msg failed"
            ",section=%s,index=%d,ip_idx=%d,ret=%d,uin=%u,errno=%d\n"
            , section_, index, ip_idx, ret, uin, errno));
        //关闭连接，清理状态
        fini(index, ip_idx);
        Stat::instance()->incre_stat(STAT_CODE_SEND_FAIL);
        return ret;
    }
    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx send_recv_ex send msg succ"
        ",index=%d,ip_idx=%d,ret=%d,uin=%u\n", index, ip_idx, ret, uin));
	// 
    ret = conn_[ip_idx][index]->recv(recv_buf, recv_len, &time_out_);
    if (ret <= 0) //异常或者对端关闭
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx send_recv_ex recv msg failed"
            ",section=%s,index=%d,ip_idx=%d,ret=%d,uin=%u,errno=%d\n"
            , section_, index, ip_idx, ret, uin, errno));
        //关闭连接，清理状态
        fini(index, ip_idx);
        Stat::instance()->incre_stat(STAT_CODE_RECV_FAIL);
        return ret;
    }

    // 判断结束符 
    if (separator != NULL)
    {
        int tmp_recv_len = ret;
        //判断消息是否结束
        while (strstr((const char*)recv_buf, separator) == NULL 
            && tmp_recv_len < max_recv_len) //未结束，继续接收
        {
            ret = conn_[ip_idx][index]->recv((char*)recv_buf + tmp_recv_len
                , max_recv_len - tmp_recv_len, &time_out_);
            if (ret <= 0) //异常或者对端关闭
            {
                ACE_DEBUG((LM_ERROR, "[%D] [%N,%l]PlatConnMgrEx send_recv_ex"
                    " recv msg failed"
                    ",section=%s,index=%d,ip_idx=%d,ret=%d,uin=%u,errno=%d\n"
                    , section_, index, ip_idx, ret, uin, errno));
                //关闭连接，清理状态
                fini(index, ip_idx);
                Stat::instance()->incre_stat(STAT_CODE_RECV_FAIL);
                return ret;
            }
            tmp_recv_len += ret;
            ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx send_recv_ex msg"
                ",index=%d,ip_idx=%d,ret=%d,uin=%u\n", index, ip_idx, ret, uin));
        }
        ret = tmp_recv_len;
    }

    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx send_recv_ex msg succ"
        ",index=%d,ip_idx=%d,ret=%d,uin=%u\n", index, ip_idx, ret, uin));
    
    conn_use_flag_[ip_idx][index] = 0;//退出前清理使用状态
    return ret;
}

int PlatConnMgrEx::recv(void * buf, int len, const unsigned int uin)
{
    int ip_idx = get_ip_idx(uin); 
    int index = get_index(ip_idx); 
    ACE_SOCK_Stream* conn = get_conn(index, ip_idx);
    if (conn == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx get conn failed"
            ",can't get a useful conn"
            ",section=%s,index=%d,ip_idx=%d,uin=%u\n"
            , section_, index, ip_idx, uin
            ));
        return -1;
    }
    ACE_Guard<ACE_Thread_Mutex> guard(conn_lock_[ip_idx][index]);// 加锁需要放在获取的后面 
    if (conn_[ip_idx][index] == NULL) // 加锁后增加一次非法判断 
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx recv failed"
            ",conn is null"
            ",section=%s,index=%d,ip_idx=%d,uin=%u\n"
            , section_, index, ip_idx, uin
            ));
        return -1;
    }
    conn_use_flag_[ip_idx][index] = 1;
    
    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx start to recv msg"
        ",index=%d,ip_idx=%d,uin=%u\n", index, ip_idx, uin));
    int ret = conn_[ip_idx][index]->recv(buf, len, &time_out_);
    if (ret <= 0) //异常或者对端关闭
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx recv msg failed"
            ",section=%s,index=%d,ip_idx=%d,ret=%d,uin=%u,errno=%d\n"
            , section_, index, ip_idx, ret, uin, errno));
        //关闭连接
        fini(index, ip_idx);
        Stat::instance()->incre_stat(STAT_CODE_RECV_FAIL);
        return ret;
    }
    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx recv msg succ"
        ",index=%d,ip_idx=%d,ret=%d,uin=%u\n", index, ip_idx, ret, uin));

    conn_use_flag_[ip_idx][index] = 0;//退出前清理使用状态
    return ret;
}

ACE_SOCK_Stream* PlatConnMgrEx::get_conn(int index, int & ip_idx) //从连接池获取一个连接
{    
    ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx start to get conn"
        ",index=%d,ip_idx=%d\n", index, ip_idx));
	if (index<0 || index>=POOL_CONN_MAX || ip_idx<0 || ip_idx >=IP_NUM_MAX)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx get conn failed, para is invalid"
            ",section=%s,index=%d,ip_idx=%d\n", section_, index, ip_idx));
        return NULL;
    }
    
    // 连接策略 
    if (use_strategy_ == 1 //使用策略
        && fail_times_[ip_idx] > max_fail_times_  //失败次数大于最大允许失败次数
        && (time(0) - last_fail_time_[ip_idx]) < recon_interval_ //时间间隔小于重连时间间隔 
    )
    {
        ACE_DEBUG((LM_TRACE, "[%D] PlatConnMgrEx get conn failed,because of strategy"
            ",section=%s"
            ",index=%d"
            ",ip_idx=%d"
            ",fail_times=%d"
            ",last_fail_time=%d"
			"\n"
            , section_
            , index
            , ip_idx
            , fail_times_[ip_idx]
            , last_fail_time_[ip_idx]
            ));
        // 尝试下一个 ip 
        ip_idx = (++ip_idx)%ip_num_;
        
        // 尝试下一个的时候不做任何重连操作 直接使用 避免下一个也异常 出现频繁重连的情况
        return conn_[ip_idx][index]; // NULL; //
    }
    
    if (conn_[ip_idx][index] == NULL)
    {
        // 保证每次只有一个线程在初始化
        ACE_Guard<ACE_Thread_Mutex> guard(conn_lock_[ip_idx][index]);
        if (conn_[ip_idx][index] == NULL)
        {
            init_conn(index, ip_idx);
        }
    }
    
    return conn_[ip_idx][index];
}

//系统正常运行之后调用此接口说明连接有问题
int PlatConnMgrEx::fini(int index, int ip_idx)
{    
    if (index<0 || index>=POOL_CONN_MAX || ip_idx<0 || ip_idx >=IP_NUM_MAX)
    {
        return -1;
    }
    
    // 连接策略 
    fail_times_[ip_idx]++;
	last_fail_time_[ip_idx] = time(0);
    
    if (conn_[ip_idx][index] != NULL)
    {
        conn_[ip_idx][index]->close();
        delete conn_[ip_idx][index];
        conn_[ip_idx][index] = NULL;
    }
    conn_use_flag_[ip_idx][index] = 0;
    
    ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrEx fini conn succ"
		",ip=%s"
		",port=%d"
		",index=%d"
		",ip_idx=%d"
		",fail_times=%d"
		"\n"
		, ip_[ip_idx]
		, port_
		, index
		, ip_idx
		, fail_times_[ip_idx]
		));

    return 0;
}

// 返回连接 index 的算法是相对比较粗略的，不会那么严格，
// 但是连接使用的时候会通过加锁控制，一个连接同时只能一个线程使用
unsigned int PlatConnMgrEx::get_index(int ip_idx, unsigned uin)
{
    ip_idx = ip_idx % ip_num_;

#ifdef THREAD_BIND_CONN
    // 如果采用 getpid() suse下编译获取到的pid为同一个 
    // ACE_OS::thr_self()
    return syscall(SYS_gettid)%conn_nums_;//此算法依赖于线程号的顺序产生才能均匀分配请求         
#else
    // 先随机看看是否有空的    
    int index = 0;
    if(0==uin) index = rand()%conn_nums_;
    else index= (uin/100)%conn_nums_;
    
    if(0 == conn_use_flag_[ip_idx][index])
    {
        return index;
    }
    
    //否则找到一个没有使用的连接就返回:
    int i = 0;
    for(i=0; i<conn_nums_; i++)
    {
        if(0 == conn_use_flag_[ip_idx][i])
        {
            index = i;
            break;
        }
    }
    //如果没找到，则仍然返回刚才随机的，因为连接使用时本身会加锁，不会有太大影响
    if(i == conn_nums_)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrEx get_index conn failed,runout"
    		",ip=%s"
    		",port=%d"
    		",index=%d"
    		"\n"
    		, ip_[ip_idx]
    		, port_
    		, index
    		));
        Stat::instance()->incre_stat(STAT_CODE_TCP_CONN_RUNOUT);
    }
    return index;

#endif
}	

// 尽量保证路由是随机的 不然很容易导致集中访问到某个 ip 上面 
int PlatConnMgrEx::get_ip_idx(unsigned int uin)
{
    if (uin == 0)
    {
        return rand()%ip_num_;
    }
    
    return uin%ip_num_;
}

int PlatConnMgrEx::get_ip_idx(const std::string& ip)
{
    for(int i = 0; i < ip_num_; ++i)
    {
        if(ip.compare(ip_[i]) == 0)
        {
            return i;
        }
    }

    return rand()%ip_num_;
}

