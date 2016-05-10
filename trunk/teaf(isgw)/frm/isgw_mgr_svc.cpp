#include "qmode_msg.h"
#include "isgw_mgr_svc.h"
#include "isgw_ack.h" //回送客户端响应消息
#include "stat.h"
#include "isgw_oper_base.h"
#include "cmd_amount_contrl.h"

#ifdef ISGW_USE_LUA
#include "lua_oper.h"
#endif

extern IsgwOperBase* factory_method();

int ISGWMgrSvc::discard_flag_ = 1; //默认进行丢弃 
int ISGWMgrSvc::discard_time_ = 2; //默认超过2秒丢弃

int ISGWMgrSvc::control_flag_ = 0;
CmdAmntCntrl *ISGWMgrSvc::freq_obj_=NULL;

ISGWMgrSvc* ISGWMgrSvc::instance_ = NULL;
int ISGWMgrSvc::rflag_=0;
int ISGWMgrSvc::ripnum_=0;

map<string, PlatConnMgrAsy*> ISGWMgrSvc::route_conn_mgr_map_;
ACE_Thread_Mutex ISGWMgrSvc::conn_mgr_lock_;
int ISGWMgrSvc::local_port_ = 0; // 本地端口

ISGWMgrSvc* ISGWMgrSvc::instance()
{
    if (instance_ == NULL)
    {
        instance_ = new ISGWMgrSvc();
    }
    return instance_;
}

int ISGWMgrSvc::init()
{
    ACE_DEBUG((LM_INFO,
		"[%D] ISGWMgrSvc start to init\n"
		));

    int ret;
    //调用各业务定义的工厂方法，生产出一个子类的对象
    //如果子类的对象生产失败，则程序启动错误
    IsgwOperBase *object = factory_method();
    if(NULL == object)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc init IsgwOperBase failed\n"));
        return -1;
    }

    ret =IsgwOperBase::instance(object)->init();
    if(ret !=0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc init IsgwOperBase failed\n"));
        return -1;
    }

// 通过 dll 方式 加载 dll 并执行初始化函数 
// 目前动态库的实现有问题，不可用
// 不能静态编译，不然会core
#if defined ISGW_USE_DLL
    ret = init_dll_os(NULL);
    if (ret != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc init dll failed\n"));
        return -1;
    }
#elif defined ISGW_USE_LUA
    //实例化一下，避免产生多个对象
    LuaOper::instance();
#endif

    //消息转发的设置
    //rflag_=0,关闭路由功能
    //rflag_=1,打开路由功能，并且只做路由转发
    //rflag_=2,打开路由功能，并且也做消息处理
    SysConf::instance()->get_conf_int("router", "route_flag", &rflag_);
    int idx=0;
    char tmp_str[32];
    while(1)
    {
        char ip[16] = {0};
        int port = 0;
        snprintf(tmp_str, sizeof(tmp_str), "ip_%d", idx);
        if(0!=SysConf::instance()->get_conf_str("router", tmp_str, ip, sizeof(ip)))
            break;
            
        snprintf(tmp_str, sizeof(tmp_str), "port_%d", idx);
        if(0!=SysConf::instance()->get_conf_int("router", tmp_str, &port))
            break;        
        
        PlatConnMgrAsy *mgr =new PlatConnMgrAsy(ip, port);
        char appname[32] = {0};
        snprintf(appname, sizeof(appname), "router_%d", idx);        
        route_conn_mgr_map_[appname] = mgr;
        
        idx++;
    }
    ripnum_ = idx; // 记录配置的ip数量 

    // 本地端口
    SysConf::instance()->get_conf_int("system", "port", &local_port_);
    
    //流量控制的设置
    SysConf::instance()->get_conf_int("cmd_amnt_cntrl", "control_flag", &control_flag_);
    freq_obj_ = new CmdAmntCntrl();
    
    // 超时设置 
    SysConf::instance()->get_conf_int("isgw_mgr_svc", "discard_flag", &discard_flag_);
    SysConf::instance()->get_conf_int("isgw_mgr_svc", "discard_time", &discard_time_);

    int quesize = MSG_QUE_SIZE;	
    //设置消息队列大小
    SysConf::instance()->get_conf_int("isgw_mgr_svc", "quesize", &quesize); 
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc set quesize=%d(byte)\n", quesize));
    open(quesize, quesize, NULL); 
    
    //设置线程数，默认为 10 
    int threads = DEFAULT_THREADS; // ACESvc 的 常量 
    SysConf::instance()->get_conf_int("isgw_mgr_svc", "threads", &threads);    
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc set number of threads=%d\n", threads));
	//激活线程
    activate(THR_NEW_LWP | THR_JOINABLE, threads);

    ACE_DEBUG((LM_INFO,
    	"[%D] ISGWMgrSvc init succ,inner lock=0x%x,out lock=0x%x\n"
    	, &(queue_.lock())
        , &(queue_lock_.lock())
    	));
    return 0;
}

#ifdef ISGW_USE_DLL
// ace 的 dll 操作方式 
int ISGWMgrSvc::init_dll(const char* dllname)
{
    if (dllname != NULL && strlen(dllname) != 0)
    {
        strncpy(dllname_, dllname, sizeof(dllname_));
    }
    
    if (strlen(dllname_) == 0)
    {
        if (SysConf::instance()
            ->get_conf_str("isgw_mgr_svc", "dllname", dllname_, sizeof(dllname_)) != 0)
        {
            ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get dllname failed\n"));
            return -1;
        }
    }

    // 强制卸载 dll 
    dll_.close ();
    int ret = dll_.open(dllname_);
    if (ret != 0) 
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc open dll %s failed"
            ",%s\n"
            , dllname_
            , dll_.error()
            ));
        return -1;
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc open dll %s succ\n", dllname_));    
    
    OPER_INIT oper_init = (OPER_INIT)dll_.symbol("oper_init");
    if (oper_init == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get symbol oper_init failed\n"));
        return -1;
    }
    oper_init();
    oper_proc_ = (OPER_PROC)dll_.symbol("oper_proc");
    if (oper_proc_ == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get symbol oper_proc failed\n"));
        return -1;
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc get all symbol succ\n"));
    
    return 0;
}

// os 自带的 dll 操作方式 
int ISGWMgrSvc::init_dll_os(const char* dllname)
{
    if (dllname != NULL && strlen(dllname) != 0)
    {
        strncpy(dllname_, dllname, sizeof(dllname_));
    }
    
    if (strlen(dllname_) == 0)
    {
        if (SysConf::instance()
            ->get_conf_str("isgw_mgr_svc", "dllname", dllname_, sizeof(dllname_)) != 0)
        {
            ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get dllname failed\n"));
            return -1;
        }
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc init_dll_os %s start\n", dllname_));
    
    // 强制卸载 dll 
    if (dll_hdl_ != ACE_SHLIB_INVALID_HANDLE) 
    {
        ACE_OS::dlclose(dll_hdl_);
    }
    
    dll_hdl_ = ACE_OS::dlopen(dllname_);
    if (dll_hdl_ == ACE_SHLIB_INVALID_HANDLE) 
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc open dll %s failed"
            ",%s\n"
            , dllname_
            , ACE_OS::dlerror()
            ));
        return -1;
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc open dll %s succ\n", dllname_));
    
    OPER_INIT oper_init = (OPER_INIT)ACE_OS::dlsym(dll_hdl_, "oper_init");
    if (oper_init == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get symbol oper_init failed\n"));
        return -1;
    }
    oper_init();
    oper_proc_ = (OPER_PROC)ACE_OS::dlsym(dll_hdl_, "oper_proc");
    if (oper_proc_ == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get symbol oper_proc failed\n"));
        return -1;
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc get all symbol succ\n"));
    
    return 0;
}
#endif

PPMsg* ISGWMgrSvc::process(PPMsg*& ppmsg)
{
    unsigned int threadid = syscall(__NR_gettid); //ACE_OS::thr_self();
    unsigned int pid = ACE_OS::getpid();

    ACE_DEBUG((LM_TRACE, "[%D] ISGWMgrSvc (%u:%u) process start\n", threadid, pid));

    //把请求转换成qmode消息方便操作
    //二进制需要传入长度
    #ifdef BINARY_PROTOCOL
    QModeMsg qmode_req(ppmsg->msg_len, ppmsg->msg, ppmsg->sock_fd, ppmsg->sock_seq
        , ppmsg->seq_no, ppmsg->protocol, (unsigned int)ppmsg->tv_time.tv_sec, ppmsg->sock_ip, ppmsg->sock_port);
    #else
    QModeMsg qmode_req(ppmsg->msg, ppmsg->sock_fd, ppmsg->sock_seq
        , ppmsg->seq_no, ppmsg->protocol, (unsigned int)ppmsg->tv_time.tv_sec, ppmsg->sock_ip, ppmsg->sock_port);
    #endif
    qmode_req.set_tvtime(&(ppmsg->tv_time));

    ppmsg->cmd = qmode_req.get_cmd();
    ppmsg->msg_len = 0;
    //上报请求量统计 ，此处只统计请求量，不包含其他指标
    ReprtInfo info(ppmsg->cmd, 1, 0, 0, 0);
    Stat::instance()->add_stat(&info);
    
    //回收请求消息资源
    //ISGW_Object_Que<PriProReq>::instance()->enqueue(pp_req, pp_req->index);
    //pp_req = NULL;
    
    int ret = 0; 
    ///ack_len 用来控制以什么协议，是否回送响应消息给客户端等等，具体取值如下:
    ///此值为正(MAX_INNER_MSG_LEN除外)按照业务指定的长度传输数据，由业务保证协议的完整性，
    ///此值为负表示不给客户端返回响应消息
    ///0或MAX_INNER_MSG_LEN 值由框架本身补充协议的完整性，比如加结束符MSG_SEPARATOR
    int ack_len = MAX_INNER_MSG_LEN; 
    
    // 内部命令的处理过程 优先处理 不检查合法性 
    if (qmode_req.get_cmd() <100 && qmode_req.get_cmd() >0)
    {
        char ack_intnl[MAX_INNER_MSG_LEN] = {0};
        ret = IsgwOperBase::instance()->internal_process(qmode_req, ack_intnl, ack_len);
        snprintf(ppmsg->msg, MAX_INNER_MSG_LEN, "%s=%d&%s=%d&%s%s"
            , FIELD_NAME_CMD, qmode_req.get_cmd(), FIELD_NAME_RESULT, ret
            , ack_intnl, MSG_SEPARATOR);

        if (ack_len < 0) //不向客户端返回响应消息
        {
            //回收响应消息资源
            ISGW_Object_Que<PPMsg>::instance()->enqueue(ppmsg, ppmsg->index);
            ppmsg = NULL;
        }
        return ppmsg;
    }
    
    // 检查合法性 
    ret = check(qmode_req);
    if (ret != 0)
    {
        ppmsg->ret = ret;
        // 超时则返回原消息 并且 result 为 ret 的值 
        snprintf(ppmsg->msg, MAX_INNER_MSG_LEN, "%s=%d&%s%s"
            , FIELD_NAME_RESULT, ret, qmode_req.get_body()
            , MSG_SEPARATOR);
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc process check msg failed,ret=%d"
            ",sock_fd=%u,prot=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u,time=%u,now=%u"
            ",msg=%s\n"
            , ret
            , ppmsg->sock_fd
            , ppmsg->protocol
            , ppmsg->sock_ip
            , ppmsg->sock_port
            , ppmsg->sock_seq
            , ppmsg->seq_no
            , ppmsg->tv_time.tv_sec
            , ISGWAck::instance()->get_time()
            , ppmsg->msg
            ));

        ppmsg->ret = ret;
        return ppmsg;
    }

    ret = 0;
    char ack_buf[MAX_INNER_MSG_LEN+1] = {0}; // 存放以地址&分割的结果信息
    
    struct timeval p_start, p_end;
    gettimeofday(&p_start, NULL);
    unsigned diff = EASY_UTIL::get_span(&(ppmsg->tv_time), &p_start);
    //根据客户端请求指令进行相关操作
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWMgrSvc start to process cmd=%d,time_diff=%u"
        ",sock_fd=%u,prot=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u\n"
        , qmode_req.get_cmd()
        , diff
        , ppmsg->sock_fd
        , ppmsg->protocol
        , ppmsg->sock_ip
        , ppmsg->sock_port
        , ppmsg->sock_seq
        , ppmsg->seq_no
        ));
    //是否路由的标志，优先使用协议指定的
    int rflag = qmode_req.get_rflag();
    if(rflag == 0)
    {
        rflag = rflag_;
    }
#if defined ISGW_USE_DLL
    switch(qmode_req.get_cmd())
    {
        case CMD_FLASH_DLL:
            ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc process start init dll\n"));
            ret = init_dll_os((*(qmode_req.get_map()))["dllname"].c_str());
            if (ret != 0)
            {
                snprintf(ack_buf, ack_len-1, "%s", "info=svc init dll failed");
                ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc process init dll %s failed\n"
                    , (*(qmode_req.get_map()))["dllname"].c_str()
                    ));
            }
        break;
        default:
            if (oper_proc_ != NULL)
            {
                ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc process oper_proc,cmd=%d,uin=%u\n"
					, qmode_req.get_cmd(), qmode_req.get_uin()));
                ret = oper_proc_(qmode_req,  ack_buf, ack_len);
            }
            else
            {
                ret = -2;
                snprintf(ack_buf, ack_len-1, "%s", "info=svc oper_proc is null");
                ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc oper_proc failed,is null,try reinit dll\n"));
                init_dll_os(NULL);
            }
        break;
    }
#elif defined ISGW_USE_LUA
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc run lua\n"));
    LuaOper::instance()->process(qmode_req.get_body(), ack_buf, ack_len);

#else
    
    int proc_rflag = rflag;
    if (0 != proc_rflag)
    {
        string &dst_svr = (*(qmode_req.get_map()))["_appname"];
        if(!dst_svr.empty()) // 指定了转发地址
        {
            int dst_port = 0;
            // 目的地址未配置 或者 与本地地址匹配
            if(SysConf::instance()->get_conf_int(dst_svr.c_str(), "port", &dst_port) != 0
                || dst_port == local_port_)
            {
                proc_rflag = 0; // 不再转发，直接处理
            }
        }
    }

    // 配置文件(ini)未开启路由转发、指令未指定转发
    if (0 == proc_rflag)
    {
        ret = IsgwOperBase::instance()->process(qmode_req,  ack_buf, ack_len);
    }
    else if (1 == proc_rflag)
    {
        ret = forward(qmode_req, proc_rflag, qmode_req.get_uin());
        if(0 == ret) 
        {
            // 如果(只)转发则本模块在此处不会立即回客户端消息
            ack_len = -1;
        }
        else
        {
            // 立即返回转发失败
            snprintf(ack_buf, MAX_INNER_MSG_LEN-1, "forward msg failed");
        }
    }
    else
    {
        // 既处理也转发，对于转发失败不处理
        ret = IsgwOperBase::instance()->process(qmode_req,  ack_buf, ack_len);
        forward(qmode_req, proc_rflag, qmode_req.get_uin());
    }
#endif
    gettimeofday(&p_end, NULL);
    ppmsg->procs_span = EASY_UTIL::get_span(&p_start, &p_end);
    ppmsg->ret = ret;
    ppmsg->cmd = qmode_req.get_cmd();
    
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWMgrSvc finish process "
        "cmd=%d,ret=%d,ack=%s,ack_len=%d,time_diff=%u,"
        "sock_fd=%u,prot=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u\n"
        , qmode_req.get_cmd()
        , ret
        , ack_buf
        , ack_len
        , ppmsg->procs_span
        , ppmsg->sock_fd
        , ppmsg->protocol
        , ppmsg->sock_ip
        , ppmsg->sock_port
        , ppmsg->sock_seq
        , ppmsg->seq_no
        ));
    
    //根据 ack_len 的结果设置响应消息
    if (ack_len < 0) //不向客户端返回响应消息
    {
        //回收响应消息资源
        //if (ppmsg != NULL)
        //{
        ISGW_Object_Que<PPMsg>::instance()->enqueue(ppmsg, ppmsg->index);
        ppmsg = NULL;
        //}
    }
    else if (ack_len == 0 || ack_len == MAX_INNER_MSG_LEN) //按照框架定义的协议响应
    {
        int msg_len = 0;
        //透传相关的内容，支持前端异步调用，名称保持和 idip 一致 
        unsigned int sock_fd = strtoul((*(qmode_req.get_map()))["_sockfd"].c_str(), NULL, 10);
        unsigned int sock_seq = strtoul((*(qmode_req.get_map()))["_sock_seq"].c_str(), NULL, 10);
        unsigned int msg_seq = strtoul((*(qmode_req.get_map()))["_msg_seq"].c_str(), NULL, 10);
        unsigned int prot = strtoul((*(qmode_req.get_map()))["_prot"].c_str(), NULL, 10);
        unsigned int time = strtoul((*(qmode_req.get_map()))["_time"].c_str(), NULL, 10);
        //此处透传rflag，告诉客户端(也是一个服务进程)是否直接透传此消息
        //int rflag = atoi((*(qmode_req.get_map()))["_rflag"].c_str());
        if (rflag != 0 || sock_fd != 0 || sock_seq != 0 || msg_seq != 0)
        {
            msg_len += snprintf(ppmsg->msg+msg_len, MAX_INNER_MSG_LEN-msg_len
                , "_sockfd=%d&_sock_seq=%d&_msg_seq=%d&_prot=%d&_time=%d&_rflag=%d&"
                , sock_fd, sock_seq, msg_seq, prot, time, rflag);
        }
        
        if (strlen(ack_buf) == 0)
        {
            msg_len += snprintf(ppmsg->msg+msg_len, MAX_INNER_MSG_LEN-msg_len-INNER_RES_MSG_LEN
                , "%s=%d&%s=%d"
                , FIELD_NAME_CMD, qmode_req.get_cmd(), FIELD_NAME_RESULT, ret
                ); 
        }
        else
        {
            msg_len += snprintf(ppmsg->msg+msg_len, MAX_INNER_MSG_LEN-msg_len-INNER_RES_MSG_LEN
                , "%s=%d&%s=%d&%s"
                , FIELD_NAME_CMD, qmode_req.get_cmd(), FIELD_NAME_RESULT, ret
                , ack_buf); 
            if (msg_len > MAX_INNER_MSG_LEN-sizeof(MSG_SEPARATOR))
            {
                msg_len = MAX_INNER_MSG_LEN-sizeof(MSG_SEPARATOR);
            }
        }
        
        //加上公共的透传字段(业务可用)和协议结束符
        msg_len += snprintf(ppmsg->msg+msg_len, MAX_INNER_MSG_LEN-msg_len
            , "&_seqstr=%s%s", (*(qmode_req.get_map()))["_seqstr"].c_str(), MSG_SEPARATOR);
    }
    else //完全按照用户定义的协议回送消息 不附加任何内容 
    {
        if (ack_len > MAX_INNER_MSG_LEN)
        {
            ACE_DEBUG((LM_ERROR, "[%D] warning: ack msg len is limited,ack_len=%d\n", ack_len));
            ack_len = MAX_INNER_MSG_LEN;
        }
        
        ppmsg->msg_len = ack_len;
        memcpy(ppmsg->msg, ack_buf, ppmsg->msg_len);
    }
    
    // 流量控制时使用 
    if(1 == control_flag_)
    {
        freq_obj_->amount_inc(qmode_req.get_cmd(), ret);
    }
    
    ACE_DEBUG((LM_TRACE, "[%D] ISGWMgrSvc (%u:%u) process end\n", threadid, pid));
    
    return ppmsg;
}

int ISGWMgrSvc::send(PPMsg* ack)
{
    ACE_DEBUG((LM_TRACE,"[%D] ISGWMgrSvc::send start\n"));
    
    //放入回送消息管理器 负责把消息送到连接对端 
    ISGWAck::instance()->putq(ack);
    
    ACE_DEBUG((LM_TRACE,"[%D] ISGWMgrSvc::send end\n"));
    return 0;
}

int ISGWMgrSvc::check(QModeMsg& qmode_req)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    
    //判断是否需要进行流量控制
    if (1 == control_flag_)
    {
        int status = freq_obj_->get_status(qmode_req.get_cmd(), (unsigned)tv_now.tv_sec);
        if (status != 0)
        {
            // 统计失败情况 
            Stat::instance()->incre_stat(STAT_CODE_SVC_REJECT);
            return ERROR_REJECT;
        }
    }
    
    // 判断处理的消息是否已经超时 
    unsigned int span = EASY_UTIL::get_span(qmode_req.get_tvtime(), &tv_now);
    if (1==discard_flag_ && span > discard_time_*10000)
    {
        // 统计失败情况 
        ACE_DEBUG((LM_ERROR,"[%D] ISGWMgrSvc::check failed,span=%u\n", span));
        Stat::instance()->incre_stat(STAT_CODE_SVC_TIMEOUT);
        return ERROR_TIMEOUT_FRM;
    }
    
    return 0;
    
}

// 消息指定了则按消息指定的路由，否则就按配置的方式路由
int ISGWMgrSvc::forward(QModeMsg& qmode_req, int rflag, unsigned int uin)
{    
    int ret = 0;
    char buf[MAX_INNER_MSG_LEN] = {0};
    // 转发时需要带上客户端信息，以便接收到返回消息时回送到客户端
    int msg_len = 0;
    msg_len += snprintf(buf, sizeof(buf)
        , "_sockfd=%d&_sock_seq=%d&_msg_seq=%d&_prot=%d&_time=%d&_rflag=%d&%s%s"
        , qmode_req.get_handle(), qmode_req.get_sock_seq(), qmode_req.get_msg_seq()
        , qmode_req.get_prot(), qmode_req.get_time(), rflag 
        , qmode_req.get_body(), MSG_SEPARATOR);
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWMgrSvc start to forward msg,buf=%s\n", buf));
    char appname[32] = {0};
    strncpy(appname, (*(qmode_req.get_map()))["_appname"].c_str(), sizeof(appname));
    if (strlen(appname) != 0) // 用协议里面的 
    {
        if (route_conn_mgr_map_[appname] == NULL)
        {
            // 需要考虑线程安全
            ACE_Guard<ACE_Thread_Mutex> guard(conn_mgr_lock_);
            if (route_conn_mgr_map_[appname] == NULL)
            {
                PlatConnMgrAsy *mgr = new PlatConnMgrAsy();
                mgr->init(appname);            
                route_conn_mgr_map_[appname] = mgr;
            }
        }
        // 异步发送的时候 == 0 是成功的 
        if(0 > route_conn_mgr_map_[appname]->send((void*)buf, msg_len, uin))
        {
            ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc forward failed,buf=%s\n", buf));
            // 统计失败情况 
            Stat::instance()->incre_stat(STAT_CODE_SVC_FORWARD);
            ret = -1;
        }
    }
    else // 用配置的 
    {
        int fail_count = 0;
        for(int idx=0; idx<ripnum_; idx++)
        {
            snprintf(appname, sizeof(appname), "router_%d", idx);
            // 异步发送的时候 == 0 是成功的 
            if(0 > route_conn_mgr_map_[appname]->send((void*)buf, msg_len, uin))
            {
                ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc forward failed,buf=%s\n", buf));
                // 统计失败情况 
                Stat::instance()->incre_stat(STAT_CODE_SVC_FORWARD);
                fail_count++;
            }
        }

        // 未配置转发ip、转发均失败
        if(ripnum_ <= 0 || fail_count == ripnum_)
        {
            ret = -1;
        }
    }
    return ret;
}


