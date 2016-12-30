#include "rds_access.h"
#include <sstream>
#include <map>
#include "stat.h"

int RdsSvr::inited = 0;

RdsSvr::RdsSvr(string & redis_sec)
{
    init(redis_sec);
}

RdsSvr::RdsSvr()
{
}

RdsSvr::~RdsSvr()
{
}

// 没加锁 最好在单线程环境 初始化一下 
int RdsSvr::init(string redis_sec)
{
    int conn_num = 20;
    int timeout = 500;
    port_ = 6379;
    memset(passwd_, 0, sizeof(passwd_));
    memset(master_ip_, 0, sizeof(master_ip_));
    memset(slave_ip_, 0, sizeof(slave_ip_));

    SysConf::instance()->get_conf_int( redis_sec.c_str(), "time_out", (int *)&timeout);
    SysConf::instance()->get_conf_int(redis_sec.c_str(), "conn_num", (int *)&conn_num);
    SysConf::instance()->get_conf_str(redis_sec.c_str(), "slave_ip", slave_ip_, sizeof(slave_ip_));
    if (0 != SysConf::instance()->get_conf_str(redis_sec.c_str(), "master_ip", master_ip_, sizeof(master_ip_)))
    {
        ACE_DEBUG((LM_INFO, "[%D] RdsSvr::init master ip read failed\n"));
        return -1;
    }
    SysConf::instance()->get_conf_int(redis_sec.c_str(), "port", (int *)&port_);
    SysConf::instance()->get_conf_str(redis_sec.c_str(), "password", passwd_, sizeof(passwd_));

    struct timeval tv = { 0, timeout*1000 };
    timeout_ = tv;
    for(int i=0; i<conn_num; i++)
    {
        Handle h;
        h.rc = redisConnectWithTimeout(master_ip_, port_, timeout_);        
        h.mutex = new ACE_Thread_Mutex();
        h.ip = string(master_ip_);
        h.port = port_;
        h.timeout = timeout_;
        m_redis_list_.push_back(h);
        //redis有可能设置口令
        if(strlen(passwd_)>0)
            redisCommand(h.rc, "auth %s", passwd_);
    }
    if(strlen(slave_ip_)>0)
    {
        for(int i=0; i<conn_num; i++)
        {
            Handle h;
            h.rc = redisConnectWithTimeout(slave_ip_, port_, timeout_);
            h.mutex = new ACE_Thread_Mutex();
            h.ip = string(slave_ip_);
            h.port = port_;
            h.timeout = timeout_;
            s_redis_list_.push_back(h);
            //redis有可能设置口令
            if(strlen(passwd_)>0)
                redisCommand(h.rc, "auth %s", passwd_);
        }
    }
    
    ACE_DEBUG((LM_INFO
        , "[%D] RdsSvr::init succ,timeout=%d,conn_num=%d,master=%s,slave=%s\n"
        , timeout, conn_num, master_ip_, slave_ip_));
    
    inited = 1;
    return 0;
}

//flag=0,get master; flag=1,get slave; flag=2 master or slave(random)
ACE_Thread_Mutex& RdsSvr::get_handle(Handle * &h, int flag)
{
    unsigned long tid = syscall(SYS_gettid);
    //根据进程id来确定所用连接
    int idx = tid%m_redis_list_.size();

    //Handle *h;
    if(0==flag||s_redis_list_.size()==0)
        h = &(m_redis_list_[idx]);
    else if(1==flag)
        h = &(s_redis_list_[idx]);
    else
    {
        unsigned random = time(0);
        if(random%2)
            h = &(s_redis_list_[idx]);
        else
            h = &(m_redis_list_[idx]);
    }

    if(NULL==h->rc)
    {
        h->rc = redisConnectWithTimeout(h->ip.c_str(), h->port, h->timeout);
        //redis有可能设置口令
        if(strlen(passwd_)>0)
            redisCommand(h->rc, "auth %s", passwd_);
    }

    return *(h->mutex);
}

//reset rediscontext
void RdsSvr::rst_handle(Handle * &h)
{
    redisFree(h->rc);
    h->rc = NULL;
//    h->dbid = -1;
    h->dbid = 0;
    //统一在此进行失败统计  因为基本是出错了 才进行 reset
    Stat::instance()->incre_stat(STAT_CODE_RDS_RESET);
}
//select db
int RdsSvr::sel_database(Handle * &h, int dbid)
{
    if(NULL==h->rc)
        return -1;
    
    if(dbid !=h->dbid)
    {
        redisReply * reply;
        reply = (redisReply *)redisCommand(h->rc, "select %d", dbid);    
        if(h->rc->err!=0)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::sel_database failed,dbid=%d,err=%x,ip=%s:%d,errstr=%s\n"
                , dbid, h->rc->err, h->ip.c_str(),h->port, h->rc->errstr == NULL ? "" : h->rc->errstr));
            rst_handle(h);
            freeReplyObject(reply);
            Stat::instance()->incre_stat(STAT_CODE_RDS_SEL_DB);
            return -1;
        }
        ACE_DEBUG((LM_INFO, "[%D] RdsSvr::sel_database succ,dbid=%d\n", dbid));
        freeReplyObject(reply);
        h->dbid = dbid;
    }

    return 0;
}

RdsSvr &RdsSvr::instance()
{
    static RdsSvr instance_;
    if (!inited)
    {
        instance_.init();
    }    
    return instance_;
}

int RdsSvr::del_key(string dbid, const string &uin, const string &custom_key)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    string key = uin;
    key += "-";
    key += custom_key;
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_key connect redis svr failed"
            ",dbid=%s"
            ",key=%s\n"
            , dbid.c_str()
            , key.c_str()));
        return -1;
    }

    reply = (redisReply *)redisCommand(h->rc, "del %s"
                , key.c_str());
    if(h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_key ctx failed,err=%x"
            ",dbid=%s"
            ",uin=%s\n"
            , h->rc->err
            , dbid.c_str()
            , uin.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if(REDIS_REPLY_ERROR==reply->type)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_key reply error,err=%s"
            ",dbid=%s"
            ",uin=%s\n"
            , reply->str
            , dbid.c_str()
            , uin.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::del_key succ,dbid=%s,key=%s\n"
        , dbid.c_str(), key.c_str()));

    return 0;
}

int RdsSvr::expire_key(const string& dbid,
                        const string& uin,
                        const string& custom_key,
                        int seconds,
                        int flag)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    string key;;
    if (flag == 0)
    {
        key = uin;
        key += "-";
        key += custom_key;
    } 
    else
    {
        key = custom_key;
    }
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::expire_key connect redis svr failed"
            ",dbid=%s"
            ",key=%s\n"
            , dbid.c_str()
            , key.c_str()));
        return -1;
    }
    
    reply = (redisReply *)redisCommand(h->rc, "expire %s %d"
                , key.c_str()
                , seconds);
    if(h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::expire_key ctx failed,err=%x"
            ",dbid=%s"
            ",uin=%s\n"
            , h->rc->err
            , dbid.c_str()
            , uin.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if(REDIS_REPLY_ERROR==reply->type)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::expire_key reply error,err=%s"
            ",dbid=%s"
            ",uin=%s\n"
            , reply->str
            , dbid.c_str()
            , uin.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::expire_key succ,dbid=%s,key=%s,secs=%d\n"
        , dbid.c_str(), key.c_str(), seconds));

    return 0;
}

int RdsSvr::redis_time(unsigned &tv_sec, unsigned &tv_usec)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
        
    if(sel_database(h, static_cast<int>(0)))
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::redis_time connect redis svr failed\n"));
        return -1;
    }
    
    reply = (redisReply *)redisCommand(h->rc, "time");
    if(h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::redis_time ctx failed,err=%x\n"
            , h->rc->err));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if(REDIS_REPLY_ARRAY!=reply->type
        ||2!=reply->elements)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::redis_time reply error,err=%s\n"
            , reply->str));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    tv_sec = strtoul(reply->element[0]->str, NULL, 10);
    tv_usec = strtoul(reply->element[1]->str, NULL, 10);
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::redis_time succ,sec=%u,usec=%u\n"
        , tv_sec
        , tv_usec));

    return 0;
}

// 返回key的过期时间
int RdsSvr::ttl(const std::string& key)
{        
    Handle *h;
    redisReply *reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));
    
    int ret = 0;
    reply = (redisReply *)redisCommand(h->rc, "ttl %s", key.c_str());
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr ttl exec failed,key=%s\n", key.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr ttl reply error,err=%s"
            ",key=%s\n"
            , reply->str
            , key.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if(REDIS_REPLY_INTEGER==reply->type)
        ret = (int)reply->integer;
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr ttl failed"
            ",key=%s,errinfo=%s\n"
            , key.c_str()
            , reply->str));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
        
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr ttl succ,key=%s,ttl=%d\n"
        , key.c_str(), ret));
    return ret;
}

//flag=0,value为string; flag=1,value为数字
int RdsSvr::get_string_reply(Handle * &h
                            , redisReply * &reply
                            , string &key
                            , string value
                            , string expire
                            , int flag)
{
    if(value.length()==0)
        reply = (redisReply *)redisCommand(h->rc, "get %s"
                    , key.c_str());
    else if(1==flag)
    {
        int num = atoi(value.c_str());
        if(0==num)
            num = 1;
        if(num>0)
            reply = (redisReply *)redisCommand(h->rc, "incrby %s %d"
                        , key.c_str()
                        , num);
        else
            reply = (redisReply *)redisCommand(h->rc, "decrby %s %d"
                        , key.c_str()
                        , 0-num);
    }
    else if(expire.length()==0)
        reply = (redisReply *)redisCommand(h->rc, "setex %s %d %s"
                    , key.c_str()
                    , 3600*48
                    , value.c_str());
    else if(expire.compare("0")==0)
        reply = (redisReply *)redisCommand(h->rc, "set %s %s"
                    , key.c_str()
                    , value.c_str());
    else
        reply = (redisReply *)redisCommand(h->rc, "setex %s %s %s"
                    , key.c_str()
                    , expire.c_str()
                    , value.c_str());
    //reply为NULL可能表示redis svr断开连接
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_string_reply exec failed\n"));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_string_reply ctx failed,err=%x,key=%s\n"
            , h->rc->err, key.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    return 0;
}

int RdsSvr::get_string_val(string dbid,
                            const string &uin,
                            const string &custom_key,
                            string &value)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));
    
    string key = uin;
    key += "-";
    key += custom_key;
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_string_val connect redis svr failed"
            ",dbid=%s"
            ",key=%s\n"
            , dbid.c_str()
            , key.c_str()));
        return -1;
    }

    if(get_string_reply(h, reply, key)!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_string_val get reply failed,key=%s\n"
            , key.c_str()));
        return -1;
    }

    if(REDIS_REPLY_STRING==reply->type)
    {
        value = reply->str;
    }
    else if(REDIS_REPLY_INTEGER==reply->type)
    {
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "%llu", reply->integer);
        value = tmp;
    }
    else if(REDIS_REPLY_NIL==reply->type)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_string_val no exist"
            ",dbid=%s"
            ",key=%s\n"
            , dbid.c_str()
            , key.c_str()));
        freeReplyObject(reply);
        return 1;
    }
    else if(REDIS_REPLY_ERROR==reply->type)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_string_val reply error,err=%s"
            ",dbid=%s"
            ",key=%s\n"
            , reply->str
            , dbid.c_str()
            , key.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_string_val succ,dbid=%s,key=%s\n"
        , dbid.c_str(), key.c_str()));

    return 0;
}

int RdsSvr::set_string_val(string dbid,
                            const string &uin,
                            const string &custom_key,
                            const string &value,
                            const string expire)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    string key = uin;
    key += "-";
    key += custom_key;
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::set_string_val connect redis svr failed"
            ",dbid=%s"
            ",key=%s\n"
            , dbid.c_str()
            , key.c_str()));
        return -1;
    }

    if(get_string_reply(h, reply, key, value, expire)!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::set_string_val get reply failed"
            ",dbid=%s"
            ",key=%s\n"
            , dbid.c_str()
            , key.c_str()));
        return -1;
    }

    if(REDIS_REPLY_ERROR==reply->type)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::set_string_val reply error"
            ",err=%s"
            ",dbid=%s"
            ",key=%s\n"
            , reply->str
            , dbid.c_str()
            , key.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::set_string_val succ,dbid=%s,key=%s\n"
        , dbid.c_str(), key.c_str()));

    return 0;
}
//将指定key的value增减指定的数字,如果value不为数字
//就返回错误
int RdsSvr::inc_string_val(string dbid,
                            const string &uin,
                            const string &custom_key,
                            string &value)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    string key = uin;
    key += "-";
    key += custom_key;
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::inc_string_val connect redis svr failed"
            ",dbid=%s"
            ",key=%s\n"
            , dbid.c_str()
            , key.c_str()));
        return -1;
    }

    if(get_string_reply(h, reply, key, value, "", 1)!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::inc_string_val get reply failed"
            ",dbid=%s"
            ",key=%s\n"
            , dbid.c_str()
            , key.c_str()));
        return -1;
    }

    if(REDIS_REPLY_INTEGER==reply->type)
    {
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "%lld", reply->integer);
        value = tmp;
    }
    else if(REDIS_REPLY_ERROR==reply->type)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::inc_string_val reply error"
            ",err=%s"
            ",dbid=%s"
            ",key=%s\n"
            , reply->str
            , dbid.c_str()
            , key.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::inc_string_val succ,dbid=%s,key=%s\n"
        , dbid.c_str(), key.c_str()));

    return 0;
}

// 更新指定key，并原子返回原先的值
int RdsSvr::get_set(const std::string& key
    , const string& custom_key
    , const std::string& nvalue
    , std::string& ovalue)
{
    Handle *h;
    redisReply *reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));

    std::string str_key = key;
    str_key += "-";
    str_key += custom_key;

    // 填充 cmd 与 key
    std::vector<std::string> args(3);
    args[0] = "GETSET";
    args[1] = str_key;
    args[2] = nvalue;
    
    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_set exec failed,key=%s\n"
            , str_key.c_str()
            ));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_set reply error,key=%s,err=%s\n"
            , str_key.c_str()
            , reply->str
            ));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    
    if(REDIS_REPLY_STRING==reply->type)
    {
        ovalue = reply->str;
    }
    else if(REDIS_REPLY_NIL==reply->type)
    {
        // key不存在，返回空
        ovalue = "";
    }
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_set failed,key=%s,err=%s\n"
            , str_key.c_str()
            , reply->str
            ));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_set succ,key=%s,ovalue=%s,nvalue=%s\n"
        , str_key.c_str()
        , ovalue.c_str()
        , nvalue.c_str()
        ));
    return 0;
}

//flag=1表示score降序排列,否则score升序排列
int RdsSvr::get_sset_list(string dbid
                            , const string &ssid
                            , vector<SSPair> &elems
                            , int start
                            , int num
                            , int flag)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_sset_list connect redis svr failed"
            ",dbid=%s"
            ",ssid=%s\n"
            , dbid.c_str()
            , ssid.c_str()));
        return -1;
    }
    
    if(1==flag)
        reply = (redisReply *)redisCommand(h->rc, "zrevrange %s %d %d withscores"
                    , ssid.c_str()
                    , start
                    , start+num-1);
    else
        reply = (redisReply *)redisCommand(h->rc, "zrange %s %d %d withscores"
                    , ssid.c_str()
                    , start
                    , start+num-1);
    
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_sset_list exec failed"
            ",dbid=%s\n"
            , dbid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_sset_list reply error,err=%s"
            ",dbid=%s"
            ",ssid=%s\n"
            , reply->str
            , dbid.c_str()
            , ssid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if(REDIS_REPLY_ARRAY==reply->type)
    {
        if(0==reply->elements)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_sset_list no row,dbid=%s,ssid=%s\n"
                , dbid.c_str(), ssid.c_str()));
            freeReplyObject(reply);
            return 1;
        }
        else if(reply->elements%2!=0)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_sset_list failed,ret array error"
                ",dbid=%s"
                ",key=%s\n"
                , dbid.c_str()
                , ssid.c_str()));
            freeReplyObject(reply);
            return -1;
        }
        for(int idx = 0; idx < reply->elements; idx += 2)
        {
        	SSPair pair;
        	pair.member = reply->element[idx]->str;
        	pair.score = strtoll(reply->element[idx+1]->str, NULL, 10);
        	elems.push_back(pair);
        }
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_sset_list succ,dbid=%s,ssid=%s\n"
        , dbid.c_str(), ssid.c_str()));

    return 0;
}

//查询指定key下sorted-set数据的指定member的分数
int RdsSvr::get_sset_score(string dbid
                    , const string &ssid
                    , vector<SSPair> &elems)
{
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_sset_score connect redis svr failed"
            ",dbid=%s"
            ",ssid=%s\n"
            , dbid.c_str()
            , ssid.c_str()));
        return -1;
    }
    for(int idx=0; idx<elems.size(); idx++)
    {
		reply = (redisReply *)redisCommand(h->rc, "ZSCORE %s %s"
					, ssid.c_str()
					, elems[idx].member.c_str());

        if(NULL==reply)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_sset_score exec failed"
                ",dbid=%s\n"
                , dbid.c_str()));
            rst_handle(h);
            freeReplyObject(reply);
            return -1;
        }
        else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_sset_score reply error,err=%s"
                ",dbid=%s"
                ",ssid=%s\n"
                , reply->str
                , dbid.c_str()
                , ssid.c_str()));
            rst_handle(h);
            freeReplyObject(reply);
            return -1;
        }
        else if(REDIS_REPLY_STRING==reply->type)
        {
        	elems[idx].score = strtoll(reply->str, NULL, 10);
        }
        else
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_sset_score failed,row error"
                ",dbid=%s"
                ",ssid=%s"
                ",idx=%d\n"
                , dbid.c_str()
                , ssid.c_str()
                , idx));
        freeReplyObject(reply);
    }

    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_sset_score succ,dbid=%s,ssid=%s\n"
        , dbid.c_str(), ssid.c_str()));

    return 0;
}

//flag=1表示score降序排列,否则score升序排列
int RdsSvr::get_sset_rank(string dbid
                            , const string &ssid
                            , vector<SSPair> &elems
                            , int flag)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_sset_rank connect redis svr failed"
            ",dbid=%s"
            ",ssid=%s\n"
            , dbid.c_str()
            , ssid.c_str()));
        return -1;
    }

    for(int idx=0; idx<elems.size(); idx++)
    {
        if(1==flag)
            reply = (redisReply *)redisCommand(h->rc, "zrevrank %s %s"
                        , ssid.c_str()
                        , elems[idx].member.c_str());
        else
            reply = (redisReply *)redisCommand(h->rc, "zrank %s %s"
                        , ssid.c_str()
                        , elems[idx].member.c_str());
        
        if(NULL==reply)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_sset_rank exec failed"
                ",dbid=%s\n"
                , dbid.c_str()));
            rst_handle(h);
            freeReplyObject(reply);
            return -1;
        }
        else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_sset_rank reply error,err=%s"
                ",dbid=%s"
                ",ssid=%s\n"
                , reply->str
                , dbid.c_str()
                , ssid.c_str()));
            rst_handle(h);
            freeReplyObject(reply);
            return -1;
        }
        else if(REDIS_REPLY_INTEGER==reply->type)
            elems[idx].rank = reply->integer;
        else
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_sset_rank failed,row error"
                ",dbid=%s"
                ",ssid=%s"
                ",idx=%d\n"
                , dbid.c_str()
                , ssid.c_str()
                , idx));
            elems[idx].has_err = 1;
        }
        freeReplyObject(reply);
    }
        
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_sset_rank succ,dbid=%s,ssid=%s\n"
        , dbid.c_str(), ssid.c_str()));

    return 0;
}
//num返回值为成功添加的member数
int RdsSvr::set_sset_list(const string& dbid
                            , const string &ssid
                            , const vector<SSPair> &elems
                            , int &num)
{
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0 || elems.empty())
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::set_sset_list connect redis svr failed"
            ",dbid=%s"
            ",ssid=%s\n"
            , dbid.c_str()
            , ssid.c_str()));
        return -1;
    }

    // 填充 cmd 与 key
    std::vector<std::string> args(2 * elems.size() + 2);
    size_t idx = 0;
    args[idx++] = "zadd";
    args[idx++] = ssid;

    // 填充元素
    for(size_t i = 0;i < elems.size();i++)
    {
        args[idx++] = stringify(elems[i].score);
        args[idx++] = elems[i].member;
    }
    
    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::set_sset_list exec failed"
            ",dbid=%s\n"
            , dbid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::set_sset_list reply error,err=%s"
            ",dbid=%s"
            ",ssid=%s\n"
            , reply->str
            , dbid.c_str()
            , ssid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
        
    if(REDIS_REPLY_INTEGER==reply->type)
       num  = (int)reply->integer;
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::set_sset_list failed,dbid=%s"
            ",ssid=%s"
            ",errinfo=%s\n"
            , dbid.c_str()
            , ssid.c_str()
            , reply->str));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::set_sset_list succ,dbid=%s,ssid=%s,fieldkey=%s,num=%d\n"
        , dbid.c_str(), ssid.c_str(), elems[0].member.c_str(), num));

    return 0;
}

//增量的设置指定每个member的score变化值
int RdsSvr::inc_sset_list(string dbid, const string &ssid, SSPair &elem)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::inc_sset_list connect redis svr failed"
            ",dbid=%s"
            ",ssid=%s\n"
            , dbid.c_str()
            , ssid.c_str()
            ));
        return -1;
    }

    reply = (redisReply *)redisCommand(h->rc, "zincrby %s %lld %s"
        , ssid.c_str()
        , elem.score
        , elem.member.c_str());
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::inc_sset_list exec failed"
            ",dbid=%s\n"
            , dbid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::inc_sset_list reply error,err=%s"
            ",dbid=%s"
            ",ssid=%s\n"
            , reply->str
            , dbid.c_str()
            , ssid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
        
    if(REDIS_REPLY_STRING==reply->type)
       elem.score = strtoll(reply->str, NULL, 10);
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::inc_sset_list failed,dbid=%s"
            ",ssid=%s"
            ",errinfo=%s\n"
            , dbid.c_str()
            , ssid.c_str()
            , reply->str));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::inc_sset_list succ,dbid=%s,ssid=%s\n"
        , dbid.c_str()
        , ssid.c_str()
        ));

    return 0;
}

//num返回值为成功删除的member数
int RdsSvr::del_sset_list(string dbid, const string &ssid, const vector<SSPair> &elems, int &num)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0 || elems.empty())
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_sset_list connect redis svr failed"
            ",dbid=%s"
            ",ssid=%s\n"
            , dbid.c_str()
            , ssid.c_str()
            ));
        return -1;
    }

    // 填充 cmd 与 key
    std::vector<std::string> args(elems.size() + 2);
    size_t idx = 0;
    args[idx++] = "zrem";
    args[idx++] = ssid;

    // 填充元素
    for(size_t i = 0;i < elems.size();i++)
    {
        args[idx++] = elems[i].member;
    }

    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_sset_list exec failed"
            ",dbid=%s\n"
            , dbid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_sset_list reply error,err=%s"
            ",dbid=%s"
            ",ssid=%s\n"
            , reply->str
            , dbid.c_str()
            , ssid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
        
    if(REDIS_REPLY_INTEGER==reply->type)
       num  = (int)reply->integer;
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_sset_list failed"
            ",dbid=%s"
            ",key=%s"
            ",errinfo=%s\n"
            , dbid.c_str()
            , ssid.c_str()
            , reply->str
            ));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_INFO
        , "[%D] RdsSvr::del_sset_list succ,dbid=%s,ssid=%s,fieldkey=%s,num=%d\n"
        , dbid.c_str(), ssid.c_str(), elems[0].member.c_str(), num));

    return 0;
}

//返回值为指定sorted-set的成员个数
int RdsSvr::get_sset_num(string dbid, const string &ssid)
{
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_sset_num connect redis svr failed"
            ",dbid=%s\n"
            , dbid.c_str()));
        return -1;
    }

    int ret = 0;
    reply = (redisReply *)redisCommand(h->rc, "zcard %s", ssid.c_str());
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_sset_num exec failed"
            ",dbid=%s\n"
            , dbid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_sset_num reply error,err=%s"
            ",dbid=%s"
            ",ssid=%s\n"
            , reply->str
            , dbid.c_str()
            , ssid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if(REDIS_REPLY_INTEGER==reply->type)
        ret = (int)reply->integer;
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_sset_num failed"
            ",dbid=%s"
            ",ssid=%s"
            ",errinfo=%s\n"
            , dbid.c_str()
            , ssid.c_str()
            , reply->str));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
        
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_sset_num succ,dbid=%s,ssid=%s\n"
        , dbid.c_str(), ssid.c_str()));

    return ret;
}

//获取hash表中指定field的value值
//fields为传入的一个或多个hash key信息
//注意: fields不存在会返回一个NIL的元素
int RdsSvr::get_hash_field(string dbid
                            , const string &hkey
                            , const vector<string> &fields
                            , map<string, string> &value)
{
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field connect redis svr failed"
            ",dbid=%s"
            ",hkey=%s\n"
            , dbid.c_str()
            , hkey.c_str()
            ));
        return -1;
    }

    // 填充 cmd 与 key
    std::vector<const char *> argv(fields.size() + 2);
    std::vector<size_t> argvlen(argv.size());
    std::string str_cmd = "HMGET";
    argv[0] = str_cmd.data();
    argvlen[0] = str_cmd.length();
    argv[1] = hkey.data();
    argvlen[1] = hkey.length();
    
    // 填充元素
    for(size_t i = 0;i < fields.size();i++)
    {
        argv[i + 2] = fields[i].data();
        argvlen[i + 2] = fields[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field exec failed"
            ",dbid=%s\n"
            , dbid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field reply error,err=%s"
            ",dbid=%s"
            ",key=%s\n"
            , reply->str
            , dbid.c_str()
            , hkey.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
        
    if(REDIS_REPLY_ARRAY==reply->type)
    {
        // HMGET For every field that does not exist in the hash, a nil value is returned. 
        if(reply->elements!=fields.size())
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_hash_field failed,ret array size error"
                ",dbid=%s"
                ",hkey=%s\n"
                , dbid.c_str()
                , hkey.c_str()));
            freeReplyObject(reply);
            return -1;
        }

        // 提取有效元素个数
        int32_t element_num = 0;
        for(int idx=0; idx<reply->elements; idx++)
        {
            if(reply->element[idx]->type!=REDIS_REPLY_NIL)
            {
                ++element_num;
                
                string field = fields[idx];
                string val = reply->element[idx]->str;
                value.insert(make_pair(field, val));
            }
        }

        // 结果集为空
        if(element_num == 0)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_hash_field no row,dbid=%s,hkey=%s\n"
                , dbid.c_str(), hkey.c_str()));
            freeReplyObject(reply);
            return 1;
        }
    }
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field failed"
            ",hkey=%s"
            ",errinfo=%s\n"
            , hkey.c_str()
            , reply->str
            ));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_hash_field succ,hkey=%s,vsize=%u\n"
        , hkey.c_str(), value.size()));

    return 0;
}

//获取hash表中field/value列表
int RdsSvr::get_hash_field_all(string dbid
                                , const string &hkey
                                , std::map<string, string> &elems)
{
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field_all connect redis svr failed"
            ",dbid=%s"
            ",hkey=%s\n"
            , dbid.c_str()
            , hkey.c_str()
            ));
        return -1;
    }

    reply = (redisReply *)redisCommand(h->rc, "HGETALL %s", hkey.c_str());
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field_all exec failed"
            ",dbid=%s\n"
            , dbid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field_all reply error,err=%s"
            ",dbid=%s"
            ",key=%s\n"
            , reply->str
            , dbid.c_str()
            , hkey.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

        
    if(REDIS_REPLY_ARRAY==reply->type)
    {
        if(0==reply->elements)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_hash_field_all no row,dbid=%s,hkey=%s\n"
                , dbid.c_str(), hkey.c_str()));
            freeReplyObject(reply);
            return 1;
        }
        else if(reply->elements%2!=0)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_hash_field_all failed,ret array error"
                ",dbid=%s"
                ",hkey=%s\n"
                , dbid.c_str()
                , hkey.c_str()));
            freeReplyObject(reply);
            return -1;
        }
        
        for(int idx=0; idx<reply->elements; idx+=2)
        {
            string field = reply->element[idx]->str;
            string value = reply->element[idx+1]->str;
            elems.insert(make_pair(field, value));
        }
    }
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field_all failed"
            ",hkey=%s"
            ",errinfo=%s\n"
            , hkey.c_str()
            , reply->str
            ));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_hash_field_all succ,hkey=%s\n", hkey.c_str()));

    return 0;
}

//设置hash表中指定field的value值
int RdsSvr::set_hash_field(string dbid
                            , const string &hkey
                            , const std::map<string, string> &elems)
{
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::set_hash_field connect redis svr failed"
            ",dbid=%s"
            ",hkey=%s\n"
            , dbid.c_str()
            , hkey.c_str()
            ));
        return -1;
    }

    // 填充 cmd 与 key
    std::vector<std::string> args(2 * elems.size() + 2);
    size_t idx = 0;
    args[idx++] = "HMSET";
    args[idx++] = hkey;
    
    // 填充元素
    for(std::map<string, string>::const_iterator it=elems.begin(); it!=elems.end(); it++)
    {
        args[idx++] = it->first;
        args[idx++] = it->second;
    }
    
    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::set_hash_field exec failed"
            ",dbid=%s,hkey=%s\n"
            , dbid.c_str(), hkey.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::set_hash_field reply error,err=%s"
            ",dbid=%s"
            ",key=%s\n"
            , reply->str
            , dbid.c_str()
            , hkey.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::set_hash_field succ,hkey=%s\n"
        , hkey.c_str()));

    return 0;
}

//增加hash表中指定field的value值
// 返回值
// =-1表示失败
// =0表示成功, value是输入&输出参数
int RdsSvr::inc_hash_field(const string& dbid
                            , const string& hkey
                            , const string& field
                            , int& value)
{
    Handle* h = NULL;
    redisReply* reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    if(sel_database(h, atoi(dbid.c_str())) != 0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::inc_hash_field connect redis svr failed"
            ",dbid=%s"
            ",hkey=%s\n"
            , dbid.c_str()
            , hkey.c_str()
            ));
        return -1;
    }
    
    // 填充 cmd 与 key
    std::vector<std::string> args;
    args.push_back("HINCRBY");
    args.push_back(hkey);
    args.push_back(field);
    args.push_back(stringify(value));
    
    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL == reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::inc_hash_field exec failed"
            ",dbid=%s,hkey=%s,field=%s\n"
            , dbid.c_str(), hkey.c_str(), field.c_str()));
        return -1;
    }
    else if(REDIS_REPLY_INTEGER != reply->type || h->rc->err != 0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::inc_hash_field reply error,err=%s"
            ",dbid=%s"
            ",hkey=%s,field=%s\n"
            , reply->str
            , dbid.c_str()
            , hkey.c_str(), field.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    value = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::inc_hash_field succ"
        ",hkey=%s"
        ",field=%s\n"
        , hkey.c_str()
        , field.c_str()));
    return 0;
}

//删除hash表中指定field的value值
// 返回值为成功删除的个数
//这里批量删除hash key不成功
int RdsSvr::del_hash_field(string dbid
                            , const string &hkey
                            , const string &field)
{
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_hash_field connect redis svr failed"
            ",dbid=%s"
            ",hkey=%s\n"
            , dbid.c_str()
            , hkey.c_str()
            ));
        return -1;
    }

    // 填充 cmd 与 key
    std::vector<std::string> args;
    args.push_back("HDEL");
    args.push_back(hkey);
    args.push_back(field);
    
    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_hash_field exec failed"
            ",dbid=%s\n"
            , dbid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_hash_field reply error,err=%s"
            ",dbid=%s"
            ",key=%s\n"
            , reply->str
            , dbid.c_str()
            , hkey.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    
    int ret = 0;
    if(REDIS_REPLY_INTEGER==reply->type)
       ret = reply->integer;
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::del_hash_field failed"
            ",hkey=%s"
            ",field=%s"
            ",errinfo=%s\n"
            , hkey.c_str()
            , field.c_str()
            , reply->str
            ));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::del_hash_field succ,hkey=%s,field=%s\n"
        , hkey.c_str(), field.c_str()));

    return ret;
}

// 返回哈希表中field数目，失败返回-1
int RdsSvr::get_hash_field_num(const string &hkey)
{
    Handle *h;
    redisReply *reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    // 填充 cmd 与 key
    std::vector<std::string> args;
    args.push_back("HLEN");
    args.push_back(hkey);
    
    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field_num exec failed,hkey=%s\n"
            , hkey.c_str()
            ));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field_num reply error,hkey=%s,err=%s\n"
            , hkey.c_str()
            , reply->str
            ));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    
    int ret = 0;
    if(REDIS_REPLY_INTEGER==reply->type)
       ret = reply->integer;
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_hash_field_num failed,hkey=%s,err=%s\n"
            , hkey.c_str()
            , reply->str
            ));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_hash_field_num succ,hkey=%s,num=%d\n"
        , hkey.c_str()
        , ret
        ));
    return ret;
}

// 检查哈希表中是否存在field，返回值定义
// 0: 记录不存在
// 1: 记录存在
// -1: 失败
int RdsSvr::hexists(const std::string &hkey, const std::string& field)
{
    Handle *h;
    redisReply *reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    // 填充 cmd 与 key
    std::vector<std::string> args;
    args.push_back("HEXISTS");
    args.push_back(hkey);
    args.push_back(field);
    
    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::hexists exec failed,hkey=%s,field=%s\n"
            , hkey.c_str()
            , field.c_str()
            ));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::hexists reply error,hkey=%s,field=%s,err=%s\n"
            , hkey.c_str()
            , field.c_str()
            , reply->str
            ));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    
    if(reply->type != REDIS_REPLY_INTEGER)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::hexists failed,hkey=%s,field=%s,err=%s\n"
            , hkey.c_str()
            , field.c_str()
            , reply->str
            ));
        freeReplyObject(reply);
        return -1;
    }

    int32_t ret =reply->integer;
    freeReplyObject(reply);
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::hexists succ,hkey=%s,field=%s,num=%d\n"
        , hkey.c_str()
        , field.c_str()
        , ret
        ));
    return ret;
}

// 返回列表元素个数，默认dbid为0
int RdsSvr::get_list_num(const std::string& list)
{        
    Handle *h;
    redisReply *reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));
    
    int ret = 0;
    reply = (redisReply *)redisCommand(h->rc, "llen %s", list.c_str());
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_list_num exec failed,list=%s\n", list.c_str()));
        rst_handle(h);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_list_num reply failed,err=%s"
            ",list=%s\n"
            , reply->str
            , list.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if(REDIS_REPLY_INTEGER==reply->type)
        ret = (int)reply->integer;
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_list_num failed"
            ",list=%s,errinfo=%s\n"
            , list.c_str()
            , reply->str));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
        
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_list_num succ,list=%s,num=%d\n"
        , list.c_str(), ret));
    return ret;
}

// 返回列表指定区间元素
int RdsSvr::get_list_range(const std::string& list
    , std::vector<std::string>& range_vec
    , int32_t start
    , int32_t stop)
{
    Handle *h;
    redisReply *reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));
    
    int ret = 0;
    reply = (redisReply *)redisCommand(h->rc, "lrange %s %d %d"
        , list.c_str(), start, stop);
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_list_range exec failed,list=%s\n", list.c_str()));
        rst_handle(h);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::get_list_range reply failed,err=%s"
            ",list=%s\n"
            , reply->str
            , list.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if(REDIS_REPLY_ARRAY==reply->type)
    {
        if(0==reply->elements)
        {
            ACE_DEBUG((LM_ERROR
                , "[%D] RdsSvr::get_list_range no row,list=%s\n"
                , list.c_str()));
            freeReplyObject(reply);
            return 1;
        }
        
        ret = reply->elements; // 返回结果数目
        for(int32_t idx = 0;idx < reply->elements;idx++)
        {
            range_vec.push_back(reply->element[idx]->str);
        }
    }
    freeReplyObject(reply);
        
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::get_list_range succ,list=%s,num=%u\n"
        , list.c_str(), ret));
    return ret;
}

// 发送消息到指定频道
int RdsSvr::pub_msg(const std::string& channel
    , const std::string& msg)
{
    Handle *h;
    redisReply *reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));

    // 填充 cmd 与 key
    std::vector<const char *> argv(3);
    std::vector<size_t> argvlen(argv.size());
    std::string str_cmd = "publish";
    argv[0] = str_cmd.data();
    argvlen[0] = str_cmd.length();
    argv[1] = channel.data();
    argvlen[1] = channel.length();
    argv[2] = msg.data();
    argvlen[2] = msg.length();
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::pub_msg exec failed,channel=%s,msg=%s\n"
            , channel.c_str(), msg.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::pub_msg reply error,err=%s"
            ",channel=%s,msg=%s\n"
            , reply->str
            , channel.c_str(), msg.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if(REDIS_REPLY_INTEGER != reply->type)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::pub_msg failed"
            ",channel=%s,msg=%s,errinfo=%s\n"
            , channel.c_str(), msg.c_str(), reply->str));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
        
    ACE_DEBUG((LM_INFO
        , "[%D] RdsSvr::pub_msg succ,channel=%s,msg=%s\n"
        , channel.c_str(), msg.c_str()));
    return 0;
}

//执行指定的lua脚本
int RdsSvr::eval_exec(string dbid
                        , const string &script
                        , const string keys
                        , const string param
                        , int &res)
{        
    Handle *h;
    redisReply *reply = NULL;

    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));
    
    if(sel_database(h, atoi(dbid.c_str()))!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::eval_exec connect redis svr failed"
            ",dbid=%s\n"
            , dbid.c_str()
            ));
        return -1;
    }

    reply = (redisReply *)redisCommand(h->rc
        , "EVAL %s 1 %s %s "
        , script.c_str()
        , keys.c_str()
        , param.c_str());
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::eval_exec failed,dbid=%s\n"
            , dbid.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::eval_exec reply error,err=%s"
            ",dbid=%s\n"
            , reply->str
            , dbid.c_str()
            ));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    
    int ret = 0;
    if(REDIS_REPLY_INTEGER==reply->type)
       res = reply->integer;
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::eval_exec failed"
            ",hkey=%s"
            ",errinfo=%s"
            ",type=%d\n"
            , keys.c_str()
            , reply->str
            , reply->type
            ));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    
    ACE_DEBUG((LM_DEBUG
        , "[%D] RdsSvr::eval_exec succ,hkey=%s\n"
        , keys.c_str() ));

    return 0;
}

// 执行指定的lua脚本(多参数版)
// type:0动态解析脚本，1预加载的lua脚本哈希值
int RdsSvr::eval_multi_command(const string &script
    , const std::vector<std::string>& keys
    , const std::vector<std::string>& params
    , std::vector<std::string>& res
    , int32_t type)
{
    if(script.empty() || keys.empty() || keys.size() != params.size())
    {
        return ERROR_PARA_ILL;
    }
    
    Handle *h;
    redisReply *reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h));

    // 填充 cmd 与 key
    std::vector<std::string> args(1, "EVAL");
    if(type == 1) args[0] = "EVALSHA";
    args.push_back(script);
    args.push_back(stringify(keys.size()));
    args.insert(args.end(), keys.begin(), keys.end());
    args.insert(args.end(), params.begin(), params.end());
    
    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
    if(NULL==reply)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::eval_multi_command failed,script=%s\n", script.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    else if(REDIS_REPLY_ERROR==reply->type||h->rc->err!=0)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::eval_multi_command reply error,err=%s,script=%s\n"
            , reply->str, script.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }
    
    if(reply->type == REDIS_REPLY_NIL)
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::eval_multi_command reply nil,script=%s\n", script.c_str()));
        freeReplyObject(reply);
        return -1;
    }
    else if(reply->type == REDIS_REPLY_INTEGER)
    {
        res.push_back(EASY_UTIL::to_string(reply->integer));
    }
    else if(reply->type == REDIS_REPLY_STRING)
    {
        res.push_back(reply->str);
    }
    else if(REDIS_REPLY_ARRAY == reply->type)
    {
        res.assign(reply->elements, "");
        for(int idx=0; idx<reply->elements; idx++)
        {
            if(REDIS_REPLY_INTEGER == reply->element[idx]->type)
            {
                res[idx] = stringify(reply->element[idx]->integer);
            }
            else if(REDIS_REPLY_STRING == reply->element[idx]->type)
            {
                res[idx] = reply->element[idx]->str;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        ACE_DEBUG((LM_ERROR
            , "[%D] RdsSvr::eval_multi_command failed,type=%d\n", reply->type));
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    return 0;
}

int RdsSvr::push_list_value(const std::string& list_name, std::string& value, uint32_t left)
{
    Handle *h;
    redisReply *reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));
    std::string cmd_str = "";
    int ret = 0;

    if (left)
    {
        cmd_str = "LPUSH";
    }
    else
    {
        cmd_str = "RPUSH";
    }

    // 填充 cmd 与 key
    std::vector<std::string> args(3);
    args[0] = cmd_str;
    args[1] = list_name;
    args[2] = value;
    
    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));
 
    if (NULL == reply)
    {
        ACE_DEBUG((LM_ERROR, "[%D] RdsSvr::list_push exec failed,list_name=%s,value=%s\n"
            , list_name.c_str()
            , value.c_str()));
        rst_handle(h);
        return -1;
    }
    else if (REDIS_REPLY_ERROR == reply->type || h->rc->err != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] RdsSvr::list_push reply failed,errinfo=%s,errno=%d,list_name=%s,value=%s\n"
            , reply->str
            , h->rc->err
            , list_name.c_str()
            , value.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if (REDIS_REPLY_INTEGER == reply->type)
    {
        ret = (int)reply->integer;
    }
    else
    {
        ACE_DEBUG((LM_ERROR, "[%D] RdsSvr::list_push failed,list_name=%s,rtype=%d,errinfo=%s\n"
            , list_name.c_str()
            , reply->type
            , reply->str == NULL ? "" : reply->str
            ));
        freeReplyObject(reply);
        return -1;
    }
    
    ACE_DEBUG((LM_DEBUG, "[%D] RdsSvr::list_push succ,list_name=%s,value=%s,errinfo=%s,ret=%d\n"
        , list_name.c_str()
        , value.c_str()
        , reply->str == NULL ? "" : reply->str
        , ret));

    freeReplyObject(reply);
    return ret;
}

int RdsSvr::pop_list_value(const std::string& list_name, std::string& value, uint32_t left)
{
    Handle *h;
    redisReply *reply = NULL;
    ACE_Guard<ACE_Thread_Mutex> guard(get_handle(h, 2));
    std::string cmd_str = "";

    if (left)
    {
        cmd_str = "LPOP";
    }
    else
    {
        cmd_str = "RPOP";
    }

    // 填充 cmd 与 key
    std::vector<std::string> args(2);
    args[0] = cmd_str;
    args[1] = list_name;
    
    std::vector<const char *> argv(args.size());
    std::vector<size_t> argvlen(args.size());
    for(size_t i = 0;i < args.size();i++)
    {
        argv[i] = args[i].data();
        argvlen[i] = args[i].length();
    }
    reply = (redisReply *)redisCommandArgv(h->rc, argv.size(), &(argv[0]), &(argvlen[0]));

    if (NULL == reply)
    {
        ACE_DEBUG((LM_ERROR, "[%D] RdsSvr::list_pop_value exec failed,list_name=%s\n"
            , list_name.c_str()));
        rst_handle(h);
        return -1;
    }
    else if (REDIS_REPLY_ERROR == reply->type || h->rc->err != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] RdsSvr::list_pop_value reply failed,errinfo=%s,errno=%d,list_name=%s\n"
            , reply->str
            , h->rc->err
            , list_name.c_str()));
        rst_handle(h);
        freeReplyObject(reply);
        return -1;
    }

    if (REDIS_REPLY_STRING == reply->type)
    {
        value = reply->str;
    }
    else if (REDIS_REPLY_NIL == reply->type)
    {
        freeReplyObject(reply);
        return 1;
    }
    else
    {
        ACE_DEBUG((LM_ERROR, "[%D] RdsSvr::list_pop_value failed,list_name=%s,rtype=%d,errinfo=%s\n"
            , list_name.c_str()
            , reply->type
            , reply->str == NULL ? "": reply->str));
        freeReplyObject(reply);
        return -1;
    }
    
    ACE_DEBUG((LM_DEBUG, "[%D] RdsSvr::list_pop_value succ,list_name=%s,value=%s,errinfo=%s\n"
        , list_name.c_str()
        , value.c_str()
        , reply->str == NULL ? "" : reply->str));

    freeReplyObject(reply);
    return 0;
}
