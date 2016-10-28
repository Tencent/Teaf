#include "ibc_mgr_svc.h"
#include "isgw_ack.h" //回送客户端响应消息
#include "ibc_oper_fac.h" //对象创建工厂 
#include "stat.h"
#include "../comm/pp_prot.h"

IBCR_MAP IBCMgrSvc::ibcr_map_;
ACE_Thread_Mutex IBCMgrSvc::ibcr_map_lock_;
IBCMgrSvc* IBCMgrSvc::instance_ = NULL;
int IBCMgrSvc::max_ibcr_record_ = MAX_IBCR_RECORED;
int IBCMgrSvc::discard_flag_ = 1; //默认进行丢弃 
int IBCMgrSvc::discard_time_ = 2; //默认超过2秒丢弃

///默认消息对象池的大小
#ifndef OBJECT_QUEUE_SIZE
#define OBJECT_QUEUE_SIZE 5000
#endif

IBCMgrSvc* IBCMgrSvc::instance()
{
    if (instance_ == NULL)
    {
        instance_ = new IBCMgrSvc();
    }
    return instance_;
}

int IBCMgrSvc::init()
{
    ACE_DEBUG((LM_INFO,
		"[%D] IBCMgrSvc start to init\n"
		));

    // 超时设置 
    SysConf::instance()->get_conf_int("ibc_mgr_svc", "discard_flag", &discard_flag_);
    SysConf::instance()->get_conf_int("ibc_mgr_svc", "discard_time", &discard_time_);
    
    // 获取最大缓存的结果记录数 
    SysConf::instance()->get_conf_int("ibc_mgr_svc", "max_ibcr_record", &max_ibcr_record_); 
    ACE_DEBUG((LM_INFO, "[%D] IBCMgrSvc set max_ibcr_record=%d\n", max_ibcr_record_));
    
    int quesize = MSG_QUE_SIZE;
    //设置消息队列大小
    SysConf::instance()->get_conf_int("ibc_mgr_svc", "quesize", &quesize); 
    ACE_DEBUG((LM_INFO, "[%D] IBCMgrSvc set quesize=%d(byte)\n", quesize));
    open(quesize, quesize, NULL); 
    
    //设置线程数，默认为 10 
    int threads = DEFAULT_THREADS; // ACESvc 的 常量 
    SysConf::instance()->get_conf_int("ibc_mgr_svc", "threads", &threads);    
    ACE_DEBUG((LM_INFO, "[%D] IBCMgrSvc set number of threads=%d\n", threads));
	//激活线程
    activate(THR_NEW_LWP | THR_JOINABLE, threads);
    ACE_DEBUG((LM_INFO,
    	"[%D] IBCMgrSvc init succ\n"
    	));
    return 0;
}

PPMsg* IBCMgrSvc::process(QModeMsg*& req)
{
    int ret = 0;
    char ack_buf[MAX_INNER_MSG_LEN+1]; // 存放以地址&分割的结果信息
    memset(ack_buf, 0x0, sizeof(ack_buf));    
    int ack_len = MAX_INNER_MSG_LEN;

    //根据客户端请求指令进行相关操作
    ACE_DEBUG((LM_TRACE, "[%D] IBCMgrSvc process cmd %d\n"
        , req->get_cmd()));
    IBCOperBase* oper = IBCOperFac::create_oper(req->get_cmd());
    if (oper == NULL)
    {
        ret = ERROR_IBC_FAC;
        Stat::instance()->incre_stat(STAT_CODE_IBCSVC_FAC);
        return NULL;
    }

    //判断是否超时 
    unsigned int now_time = ISGWAck::instance()->get_time();
    if (discard_flag_==1 && ((now_time - req->get_time()) > discard_time_))
    {
        // 对于超时的消息则什么都不做，不向后端发
        ACE_DEBUG((LM_ERROR, "[%D] IBCMgrSvc process msg failed,time out"
            ",sock_fd=%u,prot=%u,ip=%u,sock_seq=%u,seq_no=%u,time=%u"
        	"\n"
        	, req->get_handle()
        	, req->get_prot()
        	, req->get_sock_ip()
        	, req->get_sock_seq()
        	, req->get_msg_seq()
        	, req->get_time()
            ));
        ret = ERROR_TIMEOUT_FRM;
        Stat::instance()->incre_stat(STAT_CODE_IBCSVC_TIMEOUT);
        return NULL;
    }
    else
    {
        ret = oper->process(*req, ack_buf, ack_len);
    }
    
    //查找到关联的结果
    IBCRKey key;           
    memset(&key, 0x0, sizeof(IBCRKey));
    key.sock_fd = req->get_handle();
    key.sock_seq = req->get_sock_seq();
    key.msg_seq = req->get_msg_seq();
    //加锁确保对结果的操作是原子的 
    ACE_Guard<ACE_Thread_Mutex> guard(ibcr_map_lock_);    
    PPMsg *pp_ack = NULL;
        
    IBCR_MAP::iterator it;
    IBCRValue *prvalue = NULL;
    
    it = ibcr_map_.find(key);
    if(it != ibcr_map_.end()) 
    {
        ACE_DEBUG((LM_TRACE, "[%D] IBCMgrSvc find match record\n"));        
        prvalue = &(it->second); //(IBCRValue)        
    }
    else //没找到 说明是新的记录 
    {        
        //判断结果记录数是否过多，过多则直接返回部分数据给前端，并删除这个记录
        if (ibcr_map_.size() > max_ibcr_record_)
        {
            IBCR_MAP::iterator tmp_it;
            ACE_DEBUG((LM_ERROR, "[%D] IBCMgrSvc delete some records"
                ",num=%d\n", ibcr_map_.size()
                ));
        	tmp_it = ibcr_map_.end();
        	--tmp_it;

            //申请响应消息资源 
            int index = ISGW_Object_Que<PPMsg>::instance()->dequeue(pp_ack);
            if ( pp_ack == NULL )
            {
                ACE_DEBUG((LM_ERROR,
                    "[%D] ISGWMgrSvc dequeue msg failed"
                    ",maybe system has no memory\n"
                    ));
                //函数返回的时候回收资源 
                delete req;
                req = NULL;

                delete oper;
                oper = NULL;
                
                return NULL;
            }
            memset(pp_ack, 0x0, sizeof(PPMsg));
            pp_ack->index = index;
            pp_ack->tv_time = *(req->get_tvtime());
            prvalue = &(tmp_it->second);
            // 在回送消息前，最好判断是否需要调用end()操作
            // 需要根据cmd找到解析类
            encode_ppack(tmp_it->first, prvalue, pp_ack);
            //放到客户端响应消息管理器，把响应消息发送给客户端
            ISGWAck::instance()->putq(pp_ack);
            //用完及时设置为空 不然后面有可能被控制流程重新放入队列中 
            //因为 pp_ack 最终会被 return 
            pp_ack = NULL; 
            
            ibcr_map_.erase(tmp_it);
        }
        
        //插入一个默认值的新记录，并初始化 total 的值 
        IBCRValue rvalue;
        //memset(&rvalue, 0x0, sizeof(IBCRValue));    
        rvalue.time = req->get_time();
        rvalue.cmd = req->get_cmd();
        rvalue.uin = req->get_uin();
        rvalue.total = atoi((*(req->get_map()))["total"].c_str());
        //需要透传的信息
        rvalue.sock_fd_ = strtoul((*(req->get_map()))["_sockfd"].c_str(), NULL, 10);
        rvalue.sock_seq_ = strtoul((*(req->get_map()))["_sock_seq"].c_str(), NULL, 10);
        rvalue.msg_seq_ = strtoul((*(req->get_map()))["_msg_seq"].c_str(), NULL, 10);
        rvalue.prot_ = strtoul((*(req->get_map()))["_prot"].c_str(), NULL, 10);
        rvalue.time_ = strtoul((*(req->get_map()))["_time"].c_str(), NULL, 10);
        rvalue.rflag_ = atoi((*(req->get_map()))["_rflag"].c_str());
        rvalue.endflag_ = atoi((*(req->get_map()))["endflag"].c_str());
        
        it = ibcr_map_.insert(ibcr_map_.begin(), pair<IBCRKey, IBCRValue>(key, rvalue));
        prvalue = &(it->second);
    }

    //记录状态，合并结果 返回的结果可能有非法或者失败的结果 业务需要处理异常 
    prvalue->cnum++;
    if (ret == 0)
    {
        prvalue->snum++;
    }
    ret = oper->merge(*prvalue, ack_buf);

    //判断关联记录是否已经全部处理 全部处理则返回数据给客户端 
    if ( prvalue->cnum >= prvalue->total )
    {
        //回调一下最终处理函数
        if (prvalue->endflag_ == 1)
        {
            ret = oper->end(*prvalue);
        }
        
        //申请响应消息资源         
        int index = ISGW_Object_Que<PPMsg>::instance()->dequeue(pp_ack);
        if ( pp_ack == NULL )
        {
            ACE_DEBUG((LM_ERROR,
                "[%D] ISGWMgrSvc dequeue msg failed"
                ",maybe system has no memory\n"
                ));
            //函数返回的时候回收资源 
            delete req;
            req = NULL;
            
            delete oper;
            oper = NULL;

            return NULL;
        }
    	memset(pp_ack, 0x0, sizeof(PPMsg));
        pp_ack->index = index;
        pp_ack->tv_time = *(req->get_tvtime());
        encode_ppack(key, prvalue, pp_ack);

        ACE_DEBUG((LM_INFO,"[%D] IBCMgrSvc process succ"
            ",time=%u"
            ",cmd=%u"
            ",uin=%u"
            ",total=%u"
            ",cnum=%u"
            ",snum=%u"
            ",msg_len=%u"
            ",sock_fd_=%u"
            ",sock_seq_=%u"
            ",msg_seq_=%u"
            ",prot_=%u"
            ",time_=%u"
            ",rflag_=%d"
            "\n"
            , prvalue->time
            , prvalue->cmd
            , prvalue->uin
            , prvalue->total
            , prvalue->cnum
            , prvalue->snum
            , prvalue->msg_len
            , prvalue->sock_fd_
            , prvalue->sock_seq_
            , prvalue->msg_seq_
            , prvalue->prot_
            , prvalue->time_
            , prvalue->rflag_
            ));
        
        //删除这条记录
        ibcr_map_.erase(it);
    }
    
    ACE_DEBUG((LM_TRACE,"[%D] IBCMgrSvc process cmd %d succ\n"
        , req->get_cmd()));

    //函数返回的时候回收资源 
    delete req;
    req = NULL;

    delete oper;
    oper = NULL;

    return pp_ack;
}

int IBCMgrSvc::send(PPMsg* ack)
{
    ACE_DEBUG((LM_TRACE,"[%D] in IBCMgrSvc::send\n"
		));
    
    //放到客户端响应消息管理器，把响应消息发送给客户端
    ISGWAck::instance()->putq(ack);
    
    ACE_DEBUG((LM_TRACE,"[%D] out IBCMgrSvc::send\n"));
    return 0;
}

int IBCMgrSvc::encode_ppack(const IBCRKey& key
    , IBCRValue* prvalue, PPMsg* pp_ack)
{
    pp_ack->sock_fd = key.sock_fd;
    pp_ack->sock_seq = key.sock_seq;
    pp_ack->seq_no = key.msg_seq;
    //pp_ack->time = prvalue->time;
    pp_ack->cmd = prvalue->cmd;
    
    //前部透传信息 
    if (prvalue->sock_fd_ != 0 || prvalue->sock_seq_ != 0 || prvalue->msg_seq_ != 0)
    {
        pp_ack->msg_len += snprintf(pp_ack->msg+pp_ack->msg_len
            , MAX_INNER_MSG_LEN-pp_ack->msg_len
            , "_sockfd=%d&_sock_seq=%d&_msg_seq=%d&_prot=%d&_time=%d&_rflag=%d&"
            , prvalue->sock_fd_, prvalue->sock_seq_, prvalue->msg_seq_
            , prvalue->prot_, prvalue->time_, prvalue->rflag_);
    }
    
    //返回消息给客户端的时候要带上协议头部 cmd result uin ... 和尾部 \n 
    pp_ack->msg_len += snprintf(pp_ack->msg+pp_ack->msg_len
        , MAX_INNER_MSG_LEN-pp_ack->msg_len
        , "%s=%d&%s=%d&%s=%u&total=%d&cnum=%d&snum=%d"
        , FIELD_NAME_CMD, prvalue->cmd
        , FIELD_NAME_RESULT, 0 //这个请求都认为是成功的 
        , FIELD_NAME_UIN, prvalue->uin
        , prvalue->total, prvalue->cnum, prvalue->snum
        );
    
    //消息体，优先处理list，默认记录分隔符为"|"(优化: 可指定分隔符)
    if(!prvalue->msg_list.empty())
    {
        pp_ack->msg_len += snprintf(pp_ack->msg+pp_ack->msg_len
            , MAX_INNER_MSG_LEN-pp_ack->msg_len-INNER_RES_MSG_LEN, "&info=");
        std::list<std::string>::iterator itor = prvalue->msg_list.begin();
        for(;itor != prvalue->msg_list.end();++itor)
        {
            // 防止协议超长
            if (pp_ack->msg_len + INNER_RES_MSG_LEN*4 >= MAX_INNER_MSG_LEN)
            {
                break;
            }
            pp_ack->msg_len += snprintf(pp_ack->msg+pp_ack->msg_len
                , MAX_INNER_MSG_LEN-pp_ack->msg_len-INNER_RES_MSG_LEN
                , "%s|", itor->c_str());
        }
    }
    else
    {
        pp_ack->msg_len += snprintf(pp_ack->msg+pp_ack->msg_len
            , MAX_INNER_MSG_LEN-pp_ack->msg_len-INNER_RES_MSG_LEN
            , "&info=%s", prvalue->msg);
    }
    
    if (pp_ack->msg_len > MAX_INNER_MSG_LEN-sizeof(MSG_SEPARATOR))
    {
        pp_ack->msg_len = MAX_INNER_MSG_LEN-sizeof(MSG_SEPARATOR);
    }
    
    //消息尾部 
    pp_ack->msg_len += snprintf(pp_ack->msg+pp_ack->msg_len
        , MAX_INNER_MSG_LEN-pp_ack->msg_len
        , "%s", MSG_SEPARATOR);
    
    return 0;
}
