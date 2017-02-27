/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
/************************************************************
  Copyright (C), 2008-2018
  FileName: temp_proxy.h
  Author: xwfang              Date: 2010-08-20
  Description: 异步连接管理器使用的模板样例代码
  Function List:   
  History:     
      <author>     <time>     <version >   <desc>

***********************************************************/
#ifndef _TEMP_PROXY_H_
#define _TEMP_PROXY_H_
#include "isgw_comm.h"
#include "plat_conn_mgr_asy.h"
#include "pdb_prot.h"

class TempProxy
{
public:
    TempProxy();
    ~TempProxy();
    // 测试回调 
    int test(QModeMsg &req);
    //回调 函数
    static int cb_test(QModeMsg &ack, string& content, char* ack_buf, int& ack_len);
    
public:
    static int init();
private:
    static PlatConnMgrAsy* get_conmgr(); 
    static int init_conmgr(); 

private:
    static PlatConnMgrAsy* conmgr_;
    static ACE_Thread_Mutex conmgr_lock_;
};

#endif //_TEMP_PROXY_H_
