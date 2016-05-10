/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
#ifndef ADMIN_PROXY_SYNC_H
#define	ADMIN_PROXY_SYNC_H

#include <vector>
#include <map>
#include "plat_conn_mgr_ex.h"
#include "qmode_msg.h"

// url=#128Char#
// Hi~#NickName#:<br>您订阅的《#32Char#》游戏发布了最新活动《#64Char#》，立即查看详情
struct AdminTipsParam
{
    int gid;
    int delaytime;
    std::string url;
    std::string gname;
    std::string actname;
};

class AdminProxySync
{
public:
    
    int start_tips_task(const AdminTipsParam& param);
    
public:
    static int init();
    
private:
    static PlatConnMgrEx* get_conmgr(); 
    static int init_conmgr(); 

private:
    static PlatConnMgrEx* conmgr_;
    static ACE_Thread_Mutex conmgr_lock_;
};

#endif	// ADMIN_PROXY_SYNC_H
