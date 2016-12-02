/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
/******************************************************************************
*  @file      isgw_mgr_svc.h
*  @author xwfang
*  @history 
*  
******************************************************************************/
#ifndef _ISGW_MGR_SVC_H_
#define _ISGW_MGR_SVC_H_
#include "isgw_comm.h"
#include "cmd_amount_contrl.h"
#include "plat_conn_mgr_asy.h"
#include "../comm/pp_prot.h"

#define MSG_QUE_SIZE 20*1024*1024

typedef int (*OPER_INIT)();
//typedef int (*OPER_PROC)(char* req, char* ack, int& ack_len);
typedef int (*OPER_PROC)(QModeMsg& req, char* ack, int& ack_len);

//此模块负责把从网络接收的消息调用业务逻辑提供的接口进行处理，并回送给网络收发接口
class ISGWMgrSvc : public ACESvc<PPMsg, PPMsg>
{
public:
    static ISGWMgrSvc* instance();
    virtual ~ISGWMgrSvc(){}
    virtual int init();
    friend class IsgwOperBase;
    
    // get message queue size
    size_t message_count() { return queue_[0].message_count(); }
    
protected:
    ISGWMgrSvc()
    {
#ifdef ISGW_USE_DLL
        memset(dllname_, 0x0, sizeof(dllname_));
        oper_proc_ = NULL;
        
        dll_hdl_ = ACE_SHLIB_INVALID_HANDLE;
#endif
    }
    
    virtual PPMsg* process(PPMsg*& ppmsg);
    virtual int send(PPMsg* ack);
    
    int check(QModeMsg& qmode_req);
    int forward(QModeMsg& qmode_req, int rflag, unsigned int uin=0);

#ifdef ISGW_USE_DLL
private:
    virtual int init_dll(const char* dllname);
    ACE_DLL dll_;
    char dllname_[128]; //
    OPER_PROC oper_proc_;
    
    ACE_SHLIB_HANDLE dll_hdl_;
    virtual int init_dll_os(const char* dllname);
#endif
    
private:
    static ISGWMgrSvc *instance_;
    static int discard_flag_; // 超时消息丢弃标志 
    static int discard_time_; // 超时时间判断 

    static int control_flag_; //默认不做流量控制 
    static CmdAmntCntrl *freq_obj_; //频率控制的对象

    //路由功能的设置开关
    //rflag_=0,关闭路由功能
    //rflag_=1,打开路由功能，并且只做路由转发
    //rflag_=2,打开路由功能，并且也做消息处理
    static int rflag_; //路由功能的开关
    static int rtnum_; //配置的路由的数量 
    //key:appname 
    static map<string, PlatConnMgrAsy*> route_conn_mgr_map_;
    static ACE_Thread_Mutex conn_mgr_lock_;

	static int local_port_; // 本地监听端口
};

#endif //_ISGW_MGR_SVC_H_
