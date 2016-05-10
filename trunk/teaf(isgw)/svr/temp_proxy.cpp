#include "temp_proxy.h"
#include "isgw_ack.h"
#include "asy_prot.h"
#include "asy_task.h"
using namespace EASY_UTIL;

PlatConnMgrAsy* TempProxy::conmgr_ = NULL;
ACE_Thread_Mutex TempProxy::conmgr_lock_;

/*
// 需要定义的解析函数 
#ifndef _ISGW_CINTF_PARSE_
#define _ISGW_CINTF_PARSE_
int isgw_cintf_parse(PPMsg * ack)
{
    QModeMsg qmode_ack(ack->msg);
    //如果需要直接返回给客户端可以把此字段设置为1
    ack->rflag = atoi((*(qmode_ack.get_map()))["_rflag"].c_str());
    ack->sock_fd = strtoul((*(qmode_ack.get_map()))["_sockfd"].c_str(), NULL, 10);
    ack->protocol = strtoul((*(qmode_ack.get_map()))["_prot"].c_str(), NULL, 10);
    ack->sock_seq = strtoul((*(qmode_ack.get_map()))["_sock_seq"].c_str(), NULL, 10);
    ack->seq_no = strtoul((*(qmode_ack.get_map()))["_msg_seq"].c_str(), NULL, 10);
    ack->cmd = qmode_ack.get_cmd();
    ack->time = strtoul((*(qmode_ack.get_map()))["_time"].c_str(), NULL, 10);
    return 0;
}
#endif
*/

int TempProxy::init_conmgr()
{
    ACE_DEBUG((LM_INFO,"[%D] TempProxy start to init conmgr\n"));
    //
    if (conmgr_ != NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] TempProxy delete old conmgr object\n"));
        delete conmgr_;
        conmgr_ = NULL;
    }
    
    //读配置文件获取服务器连接参数
    conmgr_ = new  PlatConnMgrAsy();
    char conf_section[32];
    memset(conf_section, 0x0, sizeof(conf_section));
    snprintf(conf_section, sizeof(conf_section), "igame_svr");
    
    if(0 != conmgr_->init(conf_section))
    {
        //必须清理一下对象 不然外面会继续使用导致 core 
        delete conmgr_;
        conmgr_ = NULL;
        return -1;
    }
    
    ACE_DEBUG((LM_INFO,"[%D] TempProxy init conmgr succ\n"));
    return 0;
}

PlatConnMgrAsy* TempProxy::get_conmgr()
{
    ACE_DEBUG((LM_TRACE, "[%D] TempProxy start to get conmgr\n"));

    ACE_Guard<ACE_Thread_Mutex> guard(conmgr_lock_);
    if (NULL == conmgr_)
    {
        init_conmgr();
    }

    ACE_DEBUG((LM_TRACE, "[%D] TempProxy get conmgr succ\n"));
    return conmgr_;
}

int TempProxy::init()
{
    ASYTask::instance()->set_proc(&TempProxy::cb_test);
    return 0;
}

TempProxy::TempProxy()
{
    
}

TempProxy::~TempProxy()
{
    //
}

// 测试异步连接管理器
int TempProxy::test(QModeMsg &req)
{
    unsigned int uin = req.get_uin();
    char req_buf[1024];
    memset(req_buf, 0, sizeof(req_buf));
    
    // 告知本进程接收到后端的返回消息是否要进行加工处理还是直接透传
    int rflag = atoi((*(req.get_map()))["rflag"].c_str());
    int timeout = atoi((*(req.get_map()))["timeout"].c_str());
    if (timeout <= 0)
    {
        timeout = 1;
    }
    
    PlatConnMgrAsy* conn_mgr = get_conmgr();
    if (NULL == conn_mgr)
    {
        ACE_DEBUG((LM_ERROR, "[%D] TempProxy test failed"
            ",conn mgr is null"
            ",uin=%u\n"
            , uin
            ));
        return -1;
    }
    //需要把消息的唯一标识 传给后端，不然无法区分回来的消息 
    snprintf(req_buf, sizeof(req_buf)-1
        , "_sockfd=%d&_sock_seq=%d&_msg_seq=%d&_prot=%d&_time=%d&_rflag=%d&cmd=%d&uin=%u\n"
    , req.get_handle()
    , req.get_sock_seq()
    , req.get_msg_seq()
    , req.get_prot()
    , req.get_time()
    , rflag, 103, uin);

    // 如果需要，设置回调相关的信息 
    ASYRMsg rmsg;
    rmsg.key.sock_fd = req.get_handle();
    rmsg.key.sock_seq = req.get_sock_seq();
    rmsg.key.msg_seq = req.get_msg_seq();
    //在init的时候设置, 不需要每次调用都设置，除非回调函数有变化 
    //rmsg.value.asy_proc = &TempProxy::cb_test;
    rmsg.value.content="test content string info";
    
    ACE_DEBUG((LM_DEBUG, "[%D] TempProxy test,uin=%u,req_buf=%s\n", uin, req_buf));

    //调用有回调参数的发送接口(针对每个消息指定回调信息)
    int ret = conn_mgr->send(req_buf, strlen(req_buf), rmsg);
    if (ret == -1) //-1才为失败
    {
        ACE_DEBUG((LM_ERROR, "[%D] TempProxy test send failed"
            ",uin=%u"
            ",req=%s\n"
            , uin
            , req_buf
            ));
        return -1;
    }
    
    ACE_DEBUG((LM_INFO, "[%D] TempProxy test send succ"
        ",uin=%u"
        ",req_buf=%s"
        "\n"
        , uin
        , req_buf
        ));
    return 0;
}

int TempProxy::cb_test(QModeMsg& ack, string& content, char* ack_buf, int& ack_len)
{
    snprintf(ack_buf, ack_len-1, "%s&info=cb_test async conn,%s\n", ack.get_body(), content.c_str());
    
    ACE_DEBUG((LM_INFO, "[%D] TempProxy cb_test, ack_buf=%s\n", ack_buf));
    return 0;
}

