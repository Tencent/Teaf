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
*  isgw 框架的 批量处理模块 
*  
******************************************************************************/
#ifndef _IBC_MGR_SVC_H_
#define _IBC_MGR_SVC_H_
#include "isgw_comm.h"
#include "ibc_prot.h"
#include "../comm/pp_prot.h"

#define MSG_QUE_SIZE 20*1024*1024
#define MAX_IBCR_RECORED 1024 //map 里面最多的记录数 

//此模块负责把从网络接收的消息调用业务逻辑提供的接口进行处理，并回送给网络收发接口
class IBCMgrSvc : public ACESvc<QModeMsg, PPMsg>
{
public:
    static IBCMgrSvc* instance();
    virtual ~IBCMgrSvc(){}
    virtual int init();
    
private:
    IBCMgrSvc(){}
    virtual PPMsg* process(QModeMsg*& req);
    virtual int send(PPMsg* ack);
    
    int encode_ppack(const IBCRKey& key
        , IBCRValue* prvalue, PPMsg* ack);
    
private:
    static IBCMgrSvc *instance_;
    static IBCR_MAP ibcr_map_;
    static ACE_Thread_Mutex ibcr_map_lock_;
    static int max_ibcr_record_;
    static int discard_flag_; // 超时消息丢弃标志 
    static int discard_time_; // 超时时间判断 
};

#endif //_IBC_MGR_SVC_H_
