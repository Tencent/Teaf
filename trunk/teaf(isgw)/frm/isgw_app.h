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
*  @file      isgw_app.h
*  @author xwfang
*  @history 
*  
******************************************************************************/
#ifndef _ISGW_APP_H_
#define _ISGW_APP_H_
#include "isgw_comm.h"

#define SYS_MODULE_VERSION "V3.3"

///默认消息对象池的大小
#ifndef OBJECT_QUEUE_SIZE
#define OBJECT_QUEUE_SIZE 3000
#endif 

#define MAX_FD_SETSIZE 10240
#define DEF_MAX_WAIT_MGR_TIME 5

class ISGWApp : public ACEApp
{
public:
    ISGWApp();
    virtual int init_app(int argc, ACE_TCHAR* argv[]);
    virtual int  init_sys_path(const char* program);
    virtual int  init_conf();
    virtual int  init_stat();
    virtual int quit_app();
    virtual void disp_version();
    
private:
    unsigned int max_wait_mgr_time_;
};

#endif  //_ISGW_APP_H_
