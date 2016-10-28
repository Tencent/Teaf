#include <assert.h>
#include "isgw_oper_base.h"
#include "isgw_sighdl.h"

#ifdef ISGW_USE_REDIS
#include "rds_access.h"
#endif

#ifdef ISGW_USE_LUA
#include "lua_oper.h"
#endif

#ifdef ISGW_USE_IBC
#include "ibc_mgr_svc.h"
#endif

IsgwOperBase* IsgwOperBase::oper_ = NULL;
int IsgwOperBase::cmd_auth_flag_ = 0;
map<int, int> IsgwOperBase::cmd_auth_map_;
int IsgwOperBase::svc_auth_flag_ = 0;
map<int, int> IsgwOperBase::svc_auth_map_ ;

IsgwOperBase::IsgwOperBase()
{
}

IsgwOperBase::~IsgwOperBase()
{
}

IsgwOperBase* IsgwOperBase::instance(IsgwOperBase* oper)
{
    assert(oper != NULL);
    
    oper_ = oper;
    return oper_;
}

IsgwOperBase* IsgwOperBase::instance()
{
    if (NULL == oper_)
    {
        //
        oper_ = new IsgwOperBase;
    }
    return oper_;
}

int IsgwOperBase::init()
{
#ifdef ISGW_USE_REDIS
    RdsSvr::instance().init();
#endif
    return 0;
}

int IsgwOperBase::process(QModeMsg& req, char* ack, int& ack_len)
{
    std::cout << "IsgwOperBase::process" << std::endl;
    return 0;
}

int IsgwOperBase::internal_process(QModeMsg& req, char* ack, int& ack_len)
{
    int ret =0;
    
    switch(req.get_cmd())
    {
        //
        case CMD_TEST:
        {
            ACE_DEBUG((LM_DEBUG, "[%D] IsgwOperBase start to process CMD_TEST\n"));
            //
#ifdef ISGW_USE_IBC
            int max = 100;
            for(int i=0; i<max; i++)
            {
                char msg[100]="";
                snprintf(msg, sizeof(msg)
                    , "cmd=%d&uin=%u&total=%d&info=%d test|"
                    , req.get_cmd()
                    , req.get_uin()
                    , max
                    , i
                    );
                
                QModeMsg *new_req = new QModeMsg(msg, req.get_handle()
                    , req.get_sock_seq(), req.get_msg_seq(), req.get_prot()
                    , req.get_time(), req.get_sock_ip());
    
                if (IBCMgrSvc::instance()->putq(new_req) == -1)
                {
                    //
                    delete new_req;
                    new_req = NULL;
                }
                
            }
            ack_len = -1;
#else
            //memset(ack,'@',ack_len);
            snprintf(ack, ack_len-1, "%s", "info=in test process in test process in test process in test process in test process in test process");
            ack_len = atoi((*(req.get_map()))["ack_len"].c_str());
#endif
            //
            //sleep(100);
            ACE_DEBUG((LM_INFO, "[%D] IsgwOperBase process CMD_TEST succ\n"));
        }
        break;
        case CMD_SELF_TEST:
        {
            QMSG_MAP& req_map = *req.get_map();
            int testlevel = 0;
            if(req_map.count("testlevel"))
            {
                testlevel = atoi(req_map["testlevel"].c_str());
            }
            int overproctime = 0;
            std::string errmsg;
            
            struct timeval start, end;
            ::gettimeofday(&start, NULL);
            ret = self_test(testlevel, errmsg);
            ::gettimeofday(&end, NULL);
            unsigned diff = EASY_UTIL::get_span(&start, &end);
            
            int self_test_time_threshold = 500;
            SysConf::instance()->get_conf_int("system", "self_test_time_threshold", &self_test_time_threshold);
            overproctime = static_cast<int>(diff) - self_test_time_threshold;
            //
            snprintf(ack, MAX_INNER_MSG_LEN, "overproctime=%d&errmsg=%s", overproctime, errmsg.c_str());
            return ret;
        }
        break;
        case CMD_GET_SERVER_VERSION:
        {
            snprintf(ack, MAX_INNER_MSG_LEN, "ver=%s", SERVER_VERSION);
            return ret;
        }
        break;
#ifdef ISGW_USE_REDIS
        case CMD_GET_REDIS:
        {
            string uin = "29320361";
            string key = "hello";
            string value;
            RdsSvr::instance().get_string_val("1", uin, key, value);
            snprintf(ack, MAX_INNER_MSG_LEN, "value=%s", value.c_str());
            return 0;
        }
        break;
#endif
        case CMD_SYS_LOAD_CONF:
            reload_config();
            break;
        case CMD_SYS_GET_CONTRL_STAT:
        {
            ACE_DEBUG((LM_INFO, "[%D] IsgwOperBase start to process CMD_SYS_GET_CONTRL_STAT\n"));
            int cmd_no = atoi((*(req.get_map()))["cmd_no"].c_str());  
            ISGWMgrSvc::freq_obj_->get_statiscs(cmd_no, ack, ack_len);
        }
        break;
        case CMD_SYS_SET_CONTRL_STAT:
        {
            int cmd_no = atoi((*(req.get_map()))["cmd_no"].c_str());  
            int status = atoi((*(req.get_map()))["status"].c_str());  
            ISGWMgrSvc::freq_obj_->set_status(cmd_no, status);
            snprintf(ack, ack_len, "info=succ");
        }
        break;
        case CMD_SYS_SET_CONTRL_SWITCH:
        {

            int flag = atoi((*(req.get_map()))["switch"].c_str());  
            if(0==flag)
            {
                ISGWMgrSvc::control_flag_ = 0;
            }
            else if(1==flag)
            {
                ISGWMgrSvc::control_flag_ = 1;
            }

            snprintf(ack, ack_len, "switch=%d&info=succ", ISGWMgrSvc::control_flag_);
        }
        break;
    }
    
    return 0;
}

int IsgwOperBase::self_test(int testlevel, std::string& msg)
{
    ACE_DEBUG((LM_INFO, "[%D] IsgwOperBase::self_test level=%d\n", testlevel));
    return 0;
}

 int IsgwOperBase::init_auth_cfg()
{
    std::string delim = ",";
    char szTmp[1024];
    
    cmd_auth_flag_ = 0;
    //
    SysConf::instance()->get_conf_int("system", "cmd_auth_flag", &cmd_auth_flag_);
    if (1 == cmd_auth_flag_)
    {
        memset(szTmp, 0x0, sizeof(szTmp));
        SysConf::instance()->get_conf_str("system", "cmd_auth_list", szTmp, sizeof(szTmp));

        std::vector<std::string> vCfgList;
        EASY_UTIL::split(std::string(szTmp), delim, vCfgList);
        ACE_DEBUG((LM_INFO, "[%D] IsgwOperBase init_auth_cfg,get cmd auth list:"));
        int idx;
        for (idx = 0; idx < vCfgList.size(); idx++)
        {
            ACE_DEBUG((LM_INFO, "%s,", vCfgList[idx].c_str()));
            cmd_auth_map_[atoi(vCfgList[idx].c_str())] = 1;
        }
        ACE_DEBUG((LM_INFO, "\n"));
    }
    
    svc_auth_flag_ = 0;
    //
    SysConf::instance()->get_conf_int("system", "svc_auth_flag", &svc_auth_flag_);
    if (1 == svc_auth_flag_)
    {
        memset(szTmp, 0x0, sizeof(szTmp));
        SysConf::instance()->get_conf_str("system", "svc_auth_list", szTmp, sizeof(szTmp));

        std::vector<std::string> vCfgList;
        EASY_UTIL::split(std::string(szTmp), delim, vCfgList);
        ACE_DEBUG((LM_INFO, "[%D] IsgwOperBase init_auth_cfg,get svc auth list:"));
        int idx;
        for (idx = 0; idx < vCfgList.size(); idx++)
        {
            ACE_DEBUG((LM_INFO, "%s,", vCfgList[idx].c_str()));
            svc_auth_map_[atoi(vCfgList[idx].c_str())] = 1;
        }    
        ACE_DEBUG((LM_INFO, "\n"));
    }    
    return 0;
}

int IsgwOperBase::is_auth(QModeMsg& req, char* ack, int& ack_len)
{
    if (1 == cmd_auth_flag_)
    {
        if (cmd_auth_map_[req.get_cmd()] != 1)
        {
            ACE_DEBUG((LM_ERROR, "[%D] IsgwOperBase auth failed"
                ",cmd %d not allowed\n"
                , req.get_cmd()
                ));
            snprintf(ack, ack_len-1,"info=invalid cmd %d", req.get_cmd());
            return -1;
        }
    }
    if (1 == svc_auth_flag_)
    {
        int iSvc = atoi(((*(req.get_map()))[FIELD_NAME_SVC]).c_str());
        if (svc_auth_map_[iSvc] != 1)
        {
            ACE_DEBUG((LM_ERROR, "[%D] IsgwOperBase auth failed"
                ",svc %d not allowed\n"
                , iSvc
                ));
            snprintf(ack, ack_len-1,"info=invalid service %d", iSvc);
            return -1;
        }
    }
    return 0;
}

//!!! 注意，此回调函数是在主线程中调用的，
//!!! 请不要做耗时的操作，不然会影响接入性能
int IsgwOperBase::reload_config()
{
    ACE_DEBUG((LM_INFO, "[%D] IsgwOperBase::reload_config\n"));
    SysConf::instance()->load_conf();
#ifdef ISGW_USE_LUA
    //关掉之后，下次会自动初始化
    LuaOper::instance()->close(NULL);
#endif

    return 0;
}

//!!! 注意，此回调函数是在主线程中调用的，
//!!! 请不要做耗时的操作，不然会影响接入性能
int IsgwOperBase::time_out()
{
    ACE_DEBUG((LM_DEBUG, "[%D] IsgwOperBase::time_out\n"));
    return 0;
}

//!!! 注意，此回调函数是在主线程中调用的，
//!!! 请不要做耗时的操作，不然会影响接入性能
int IsgwOperBase::handle_close(int fd)
{
    ACE_DEBUG((LM_DEBUG, "[%D] IsgwOperBase::handle_close,fd=%d\n",fd));
    return 0;
}

int IsgwOperBase::wait_task()
{
    ACE_DEBUG((LM_DEBUG, "[%D] IsgwOperBase::wait_task\n"));
    return 0;
}


