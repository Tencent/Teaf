#include "pdb_oper.h"
#ifdef VIP_OPER
#include "vip_db_oper.h"
#endif
#ifdef ADMIN_OPER
#include "admin_proxy_sync.h"
#endif

using namespace EASY_UTIL;

IsgwOperBase* factory_method()
{
    TempProxy::init();
    
    IsgwOperBase* obj = new PdbOper();
    return obj;
}

PdbOper::PdbOper()
{
    
}

PdbOper::~PdbOper()
{
    
}

int PdbOper::process(QModeMsg& req, char* ack, int& ack_len)
{
    int ret = 0;

    switch(req.get_cmd()) 
    {

/// VIP_OPER 的处理段
#ifdef VIP_OPER
        case CMD_QUERY_VIP:
        {
            VipDbOper oper;
            ret = oper.get_vip_info(req, ack, ack_len); //
        }
        break;
#endif
#ifdef ADMIN_OPER
                case CMD_TEST_ADMIN_STT:
                {
                    AdminProxySync oper;
                    AdminTipsParam param;
                    ret = oper.start_tips_task(param); //
                }
                break;
#endif

        // 异步测试指令 
        case CMD_TEST_CINTF:
        {
            TempProxy proxy;
            ret = proxy.test(req);
            if (ret >= 0) //向后端发送成功 
            {
                //此处不需要返回消息  后端会异步返回 
                ack_len = -1;
            }
        }
        break;
        // 普通测试指令 
        case CMD_TEST_COMM:
        {
            //memset(ack,'@',ack_len);
            snprintf(ack, ack_len-1, "%s", "info=in PdbOper test process  in PdbOper test process");
            ack_len = atoi((*(req.get_map()))["ack_len"].c_str());
            //测试用 xwfang
            //sleep(100);
            ACE_DEBUG((LM_INFO, "[%D][%P] PdbOper process CMD_TEST succ\n"));
        }
        break;
        
        default:
            ret = -1;
            snprintf(ack, ack_len-1,"info=Oper unknown cmd %d", req.get_cmd());
            ACE_DEBUG((LM_ERROR, "[%D] PdbOper process unknown cmd=%d,ip=%u\n"
                , req.get_cmd(), req.get_sock_ip()));
        break;
    }
    
    return ret;
}
