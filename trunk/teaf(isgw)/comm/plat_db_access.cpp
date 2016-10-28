#include "plat_db_access.h"
#include "stat.h"

PlatDbAccess::PlatDbAccess()
{
    for(int i=0; i<DB_CONN_MAX; i++)
    {
        db_conn_[i] = NULL;
        db_conn_flag_[i] = 0; //默认为空闲状态
    }
    
    conn_nums_ = POOL_CONN_DEF;
    memset(section_, 0x0, sizeof(section_));

    memset(db_host_, 0x0, sizeof(db_host_));
    memset(db_user_, 0x0, sizeof(db_user_));
    memset(db_pswd_, 0x0, sizeof(db_pswd_));
    port_ = 0;
    time_out_ = 2;

    use_strategy_ = 1; //默认使用
    max_fail_times_ = 5; //默认连续五次失败则等待一段时间重连
    recon_interval_ = 10; //默认10s重新连接 

    fail_times_ = 0;
    last_fail_time_ = 0;

    memset(err_msg_, 0x0, sizeof(err_msg_));	
}

PlatDbAccess::~PlatDbAccess()
{
    for(int i=0; i<DB_CONN_MAX; i++)
    {
        if (db_conn_[i] != NULL)
        {
            delete db_conn_[i];
            db_conn_[i] = NULL;
            db_conn_flag_[i] = 0;
        }
    }
}

int PlatDbAccess::init(const char *section )
{
    if (section != NULL)
    {
        strncpy(section_, section, sizeof(section_));
    }
	
    if (SysConf::instance()->get_conf_int(section_, "conn_nums", &conn_nums_) != 0)
    {
        conn_nums_ = POOL_CONN_DEF;
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get conn_nums config failed"
            ",section=%s"
            ",use default conn_nums %d"
            "\n"
            , section_
            , conn_nums_
            ));
    }
    //配置的再大，也不能超过框架内部限制的连接数
    if (conn_nums_ > DB_CONN_MAX)
    {
        conn_nums_ = DB_CONN_MAX;
    }
    
    if (SysConf::instance()
        ->get_conf_str( section_, "ip", db_host_, sizeof(db_host_)) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get ip config failed"
            ",section=%s\n", section_)); 
        return -1;
    }
    if (SysConf::instance()
        ->get_conf_str( section_, "user", db_user_, sizeof(db_user_)) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get user config failed"
            ",section=%s\n", section_)); 
        return -1;
    }
    if (SysConf::instance()
        ->get_conf_str( section_, "passwd", db_pswd_, sizeof(db_pswd_)) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get passwd config failed"
            ",section=%s\n", section_)); 
        return -1;
    }
	if (SysConf::instance()->get_conf_int(section_, "port", &port_) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get port config failed"
            ",section=%s\n", section_)); 
    }
    
    if (SysConf::instance()->get_conf_int(section_, "timeout", &time_out_) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get timeout config failed"
            ",section=%s\n", section_)); 
    }
    
    if (SysConf::instance()->get_conf_int(section_, "use_strategy", &use_strategy_) == 0)
    {
        //使用连接策略
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get use_strategy config succ,use strategy"
            ",section=%s\n", section_));
        SysConf::instance()->get_conf_int(section_, "max_fail_times", &max_fail_times_);
        SysConf::instance()->get_conf_int(section_, "recon_interval", &recon_interval_);        
    }

    ACE_DEBUG((LM_INFO, "[%D] PlatDbAccess start to init conn"
        ",conn_nums=%d,timeout=%d\n", conn_nums_, time_out_)); 
    
    //初始化一池连接
    int ret = 0;
    for(int index=0; index<conn_nums_&&0==ret; index++)
    {
        ACE_Guard<ACE_Thread_Mutex> guard(db_conn_lock_[index]); 
        ret=init(db_host_, db_user_, db_pswd_, port_, index);        
    }
    return ret;
	
}

int PlatDbAccess::init(const char *host, const char *user, const char *passwd, int port
        , int index)
{
    if (host==NULL || user==NULL || passwd==NULL 
        || index >= DB_CONN_MAX || index <0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess init failed, para is invalid\n"));
        return -1;
    }
	
	//初始化一池连接
    if (db_conn_[index] != NULL)
    {
        fini(index);
    }

    ACE_DEBUG((LM_DEBUG, "[%D] PlatDbAccess start to connect db"
    	",host=%s"
    	",user=%s"
    	",passwd=%s"
    	",index=%d"
    	"\n"
    	, host
    	, user
    	, passwd
    	, index
    	));

    db_conn_[index] = new PlatDbConn();
    if (db_conn_[index]->connect(host, user, passwd, port, time_out_) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess connect db failed,host=%s,errmsg=%s\n"
            , host, db_conn_[index]->get_err_msg()));
        fini(index);
        return -1;
    }

    ACE_DEBUG((LM_INFO, "[%D] PlatDbAccess connect db succ"
        ",host=%s"
        ",user=%s"
        ",passwd=%s"
        ",index=%d"
        "\n"
        , host
        , user
        , passwd
        , index
        ));
    
    return 0;
}

int PlatDbAccess::set_conns_char_set(const char* character_set)
{
    int ret = 0;
    for(int i=0; i<conn_nums_&&0==ret; i++)
    {
        ACE_Guard<ACE_Thread_Mutex> guard(db_conn_lock_[i]); 
        if(db_conn_[i]!=NULL)
        ret = db_conn_[i]->set_character_set(character_set);
    }    
    return ret;
}

unsigned long PlatDbAccess::escape_string(char *to, const char *from
	, unsigned long length)
{
    unsigned long ret = 0;
    for(int i=0; i<conn_nums_&&0==ret; i++)
    {
        ACE_Guard<ACE_Thread_Mutex> guard(db_conn_lock_[i]); 
        if(db_conn_[i]!=NULL)
        ret = db_conn_[i]->escape_string(to, from, length);
    }    
    return ret;
}

unsigned long PlatDbAccess::escape_string(string & to, const string & from)
{
    int length = from.length();
    char to_tmp[length*2+1];
    int ret =  escape_string(to_tmp, from.c_str(), length);
    to = to_tmp;
    return ret;
}

int PlatDbAccess::fini(int index)
{
    if ( index < 0 || index >= DB_CONN_MAX )
    {
        return -1;
    }
	
    // 连接策略 
    fail_times_++;
    last_fail_time_ = time(0); 

    if (db_conn_[index] != NULL)
    {  
        delete db_conn_[index];
        db_conn_[index] = NULL;
        db_conn_flag_[index] = 0;
    }

    return 0;
}

//使用的时候连接状态db_conn_flag_ 设置为 1   使用完设置为 0 
//线程和连接绑定的时候不受此变量影响
int PlatDbAccess::exec_query(const char* sql, MYSQL_RES*& result_set
    , unsigned int uin)
{
    //增加 mysql 操作计时
    struct timeval s_start;
    gettimeofday(&s_start, NULL);

    int index = get_conn_index(uin);    
    ACE_Guard<ACE_Thread_Mutex> guard(db_conn_lock_[index]); 
    // 标识是否在使用 如果不使用了 请注意清理 
    db_conn_flag_[index] = 1; // 1表示正在使用
    
    ACE_DEBUG(( LM_TRACE, "[%D] PlatDbAccess exec query sql=%s\n", sql ));

    PlatDbConn* db_conn = get_db_conn(index);
    if (db_conn == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get db conn failed,section=%s,index=%d\n", section_, index));
        db_conn_flag_[index] = 0;
        return -1;
    }

    int ret = 0;    
    ret = db_conn->exec_query(sql, result_set);
    if (ret != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess exec query failed,section=%s,idx=%d,%s\n"
            , section_, index, db_conn->get_err_msg() ));
        if(ret == -1) // -1 表示服务器连接断开，尝试重连 try again
        {
            ret = db_conn->exec_query(sql, result_set);
            if(ret == -1) //重连还失败，清理这个连接对象
            {
                fini(index);
            }
        }
    }
    db_conn_flag_[index] = 0;
    return stat_return(ret, &s_start, sql);
}

int PlatDbAccess::exec_multi_query(const char* sql, vector<MYSQL_RES*>& result_set_list
    , unsigned int uin)
{
    //增加 mysql 操作计时
    struct timeval s_start;
    gettimeofday(&s_start, NULL);

    int index = get_conn_index(uin);    
    ACE_Guard<ACE_Thread_Mutex> guard(db_conn_lock_[index]); 
    // 标识是否在使用 如果不使用了 请注意清理 
    db_conn_flag_[index] = 1; // 1表示正在使用
    
    ACE_DEBUG(( LM_TRACE, "[%D] PlatDbAccess exec query sql=%s\n", sql ));

    PlatDbConn* db_conn = get_db_conn(index);
    if (db_conn == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get db conn failed,section=%s,index=%d\n", section_, index));
        db_conn_flag_[index] = 0;
        return -1;
    }

    int ret = 0;    
    ret = db_conn->exec_multi_query(sql, result_set_list);
    if (ret != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess exec query failed,section=%s,idx=%d,%s\n"
            , section_, index, db_conn->get_err_msg() ));
        if(ret == -1) // -1 表示服务器连接断开，尝试重连 try again
        {
            ret = db_conn->exec_multi_query(sql, result_set_list);
            if(ret == -1) //重连还失败，清理这个连接对象
            {
                fini(index);
            }
        }
    }
    db_conn_flag_[index] = 0;
    return stat_return(ret, &s_start, sql);
}

//使用的时候连接状态db_conn_flag_ 设置为 1   使用完设置为 0 
//线程和连接绑定的时候不受此变量影响
int PlatDbAccess::exec_update(const char* sql, int& last_insert_id
	, int& affected_rows, unsigned int uin)
{
    //增加 mysql 操作计时
    struct timeval s_start;
    gettimeofday(&s_start, NULL);

    int index = get_conn_index(uin);
    ACE_Guard<ACE_Thread_Mutex> guard(db_conn_lock_[index]); 
    // 标识是否在使用 如果不使用了 请注意清理 
	db_conn_flag_[index] = 1;
    
    ACE_DEBUG((LM_TRACE, "[%D] PlatDbAccess exec update sql=%s,conn index=%d\n", sql, index));

    PlatDbConn* db_conn = get_db_conn(index);
    if (db_conn == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get db conn failed,section=%s,index=%d\n", section_, index));
		db_conn_flag_[index] = 0;
        return -1;
    }

    int ret = 0;
    ret = db_conn->exec_update(sql, last_insert_id, affected_rows);
    if (ret != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess exec update failed,section=%s,ret=%d,%s,try again\n"
            , section_, ret, db_conn->get_err_msg() ));
        if(ret == -1) // -1 表示服务器连接断开，尝试重连 try again
        {
            ret = db_conn->exec_update(sql, last_insert_id, affected_rows);
            if(ret == -1) //重连还失败，清理这个连接对象
            {
                fini(index);
            }
        }
    }
    db_conn_flag_[index] = 0;
    return stat_return(ret, &s_start, sql);
}

int PlatDbAccess::exec_trans(const vector<string>& sqls, int & last_insert_id
    , int & affected_rows, unsigned int uin)
{
    //增加 mysql 操作计时
    struct timeval s_start;
    gettimeofday(&s_start, NULL);

    int index = get_conn_index(uin);
    ACE_Guard<ACE_Thread_Mutex> guard(db_conn_lock_[index]); 
    // 标识是否在使用 如果不使用了 请注意清理 
	db_conn_flag_[index] = 1; 
    
    ACE_DEBUG((LM_TRACE, "[%D] PlatDbAccess exec_trans sql vec size=%d,conn index=%d\n", sqls.size(), index));

    PlatDbConn* db_conn = get_db_conn(index);
    if (db_conn == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get db conn failed,index=%d\n", index));
		db_conn_flag_[index] = 0;
        return -1;
    }

    int ret = 0;
    ret = db_conn->exec_trans(sqls, last_insert_id, affected_rows);
    if (ret != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess exec_trans failed,%s\n", db_conn->get_err_msg() ));
        if(ret == -1) // -1 表示服务器连接断开
        {
            ret = db_conn->exec_trans(sqls, last_insert_id, affected_rows);
            if(ret == -1) //重连还失败，清理这个连接对象
            {
                fini(index);
            }
        }
    }
    db_conn_flag_[index] = 0;
    return stat_return(ret, &s_start);
}

int PlatDbAccess::free_result(MYSQL_RES*& game_res)
{
    if (game_res == NULL)
    {
        return 1; //空资源，不需要释放
    }
	
    mysql_free_result(game_res);
    game_res = NULL;
    return 0;
}

int PlatDbAccess::free_result(vector<MYSQL_RES*>& result_set_vec)
{
    for(int i=0; i<result_set_vec.size(); i++)
    {
        free_result(result_set_vec[i]);
    }
    return 0;
}

PlatDbConn* PlatDbAccess::get_db_conn(int index) //从连接池获取一个连接
{
    index %= conn_nums_;
    int ret = 0;

    // 连接策略 
    if (use_strategy_ == 1 //使用策略
        && fail_times_ > max_fail_times_  //失败次数大于最大失败次数
        && (time(0) - last_fail_time_) < recon_interval_ //时间间隔小于重连时间间隔 
    )
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get conn failed,because of strategy"
            ",db_host=%s"
            ",index=%d"
            ",fail_times=%d"
            ",last_fail_time=%d"
			"\n"
            , db_host_
            , index
            , fail_times_
            , last_fail_time_
            ));
        return NULL;//NULL 
    }
    
    if (db_conn_[index] == NULL)
    {
        ret = init(db_host_, db_user_, db_pswd_, port_, index);
    }

    //连接成功实际连续失败次数要清 0 	
    if (ret == 0) //说明连接成功
    {
        fail_times_ = 0; 
    }
    
    return db_conn_[index];	
}

unsigned int PlatDbAccess::get_conn_index(unsigned int uin)
{
#ifdef THREAD_BIND_CONN
    return syscall(SYS_gettid)%conn_nums_;//此算法依赖于线程号的顺序产生才能均匀分配请求     
#else
    // 按 uin 路由 
    if (uin != 0)
    {
        return uin%conn_nums_;
    }

    // 随机路由 
    int index = rand()%conn_nums_;
    if(0 == db_conn_flag_[index])
    {
        return index;
    }
    // 找到一个没有使用的连接就返回:
    int i = 0;
    for (i=0; i<conn_nums_; i++)
    {
        if (0 == db_conn_flag_[i])
        {
            index = i;
            break;
        }
    }
    // 如果没找到，则继续用刚才随机的
    if (i == conn_nums_)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess get_conn_index conn runout"
            ",section=%s"
            ",index=%d"
            "\n"
            , section_
            , index));
        Stat::instance()->incre_stat(STAT_CODE_DB_CONN_RUNOUT);
    }
    return index;

#endif
    
}

int32_t PlatDbAccess::stat_return(const int32_t result, timeval* start, const char* sql)
{
    if(result == CR_COMMANDS_OUT_OF_SYNC
        || result == 1146 // 表不存在
        || result == 1054 // 字段不匹配
        || result == 1064 // 语法错误
        || result == 1046 // db 不存在
    )
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess stat_return failed"
            ",section=%s,result=%d,sql=%s\n", section_, result, sql));
        Stat::instance()->incre_stat(STAT_CODE_DB_CRITICAL_ERROR);
    }

    timeval end;
    gettimeofday(&end, NULL);
    unsigned diff = EASY_UTIL::get_span(start, &end);
    if (diff > 2000) //单位 0.1 ms
    {
        char tmp_sql[3900] = {0}; 
        snprintf(tmp_sql, sizeof(tmp_sql)-1, "%s\n",  sql);
        strncat(tmp_sql, "\n", 1);
        
        ACE_DEBUG((LM_ERROR, "[%D] PlatDbAccess exec sql too slow"
                ",section=%s,diff=%d,sql=%s\n", section_, diff, tmp_sql));
        Stat::instance()->incre_stat(STAT_CODE_DB_TIMEOUT);
    }
    
    return result;
}

int PlatDbAccess::is_legal(const char * sql)
{
    if(strstr(sql, ";") != NULL)
    {
        return 1; //非法
    }
    if(strstr(sql, "sleep(") != NULL)
    {
        return 1; //非法
    }

    return 0;
}

