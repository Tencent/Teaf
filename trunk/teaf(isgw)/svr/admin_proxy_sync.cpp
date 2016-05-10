#include "admin_proxy_sync.h"

PlatConnMgrEx* AdminProxySync::conmgr_ = NULL;
ACE_Thread_Mutex AdminProxySync::conmgr_lock_;

int AdminProxySync::init()
{
    return init_conmgr();
}

int AdminProxySync::init_conmgr()
{
    if (conmgr_ != NULL)
    {
        delete conmgr_;
        conmgr_ = NULL;
    }
    
    //读配置文件获取服务器连接参数
    conmgr_ = new  PlatConnMgrEx();
    char conf_section[32] = {0};
    strncpy(conf_section, "admin_svr", sizeof(conf_section) - 1);    
    if(0 != conmgr_->init(conf_section))
    {
        ACE_DEBUG((LM_ERROR,"[%D] AdminProxySync init conmgr failed\n"));
        return -1;
    }
    
    ACE_DEBUG((LM_INFO,"[%D] AdminProxySync init conmgr success\n"));
    return 0;
}

PlatConnMgrEx* AdminProxySync::get_conmgr()
{
    ACE_Guard<ACE_Thread_Mutex> guard(conmgr_lock_);
    if (NULL == conmgr_)
    {
        init_conmgr();
    }
    
    return conmgr_;
}

int AdminProxySync::start_tips_task(const AdminTipsParam& param)
{
    char cmd[1024] = {0};
    snprintf(cmd, sizeof(cmd), "cmd=920&gid=%d&delaytime=%d&url=%s&gname=%s&actname=%s\n", 
        param.gid, 
        param.delaytime, 
        param.url.c_str(),
        param.gname.c_str(),
        param.actname.c_str());
    
    PlatConnMgrEx* conn_mgr = get_conmgr();
    if (NULL == conn_mgr)
    {
        ACE_DEBUG((LM_ERROR, "[%D] AdminProxySync::start_tips_task get conn mgr failed\n"));
        return -1;
    }
    
    ACE_DEBUG((LM_INFO, "[%D] AdminProxySync::start_tips_task,req=%s\n", cmd));
    char ack[8196] = {0};
    int ret = conn_mgr->send_recv_ex(cmd, strlen(cmd), ack, sizeof(ack) - 1, "\n");
    if (ret <= 0) 
    {
        ACE_DEBUG((LM_ERROR, "[%D] AdminProxySync::start_tips_task failed,send_recv error,"
            "ret=%d\n", ret));
        return -1;
    }
    
    ///把请求转换成qmode消息方便操作
    QModeMsg qmode_ack(ack);
    int result = qmode_ack.get_result();
    if (result != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] AdminProxySync::start_tips_task failed,ack=%s\n", ack));
    }
    
    return result;
}
