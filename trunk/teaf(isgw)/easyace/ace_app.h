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
*  @file      ace_app.h
*  @author rong
*  @history 
*   200503 init rong ACE后台进程的主控制模块,对进程进行启停等
*   200605 xwfang 调整了部分模块的顺序， 增加了reactor初始化
*   的接口，使得用户能使用自己的reactor
*  
******************************************************************************/
#ifndef _ACE_APP_H_
#define _ACE_APP_H_

#include "ace_all.h"
#include "ace_conf.h"

#ifndef MAX_FD_SETSIZE 
#define MAX_FD_SETSIZE 10240
#endif

class ACEApp
{
public:
    virtual ~ACEApp();
    int init(int argc, ACE_TCHAR* argv[]);
    virtual void disp_version();
    virtual int init_app(int argc, ACE_TCHAR* argv[]);
    virtual int init_reactor();
    virtual int  init_sys_path(const char* program);
    virtual int  init_conf();
    virtual void child_main();
    virtual void quit_child();
    virtual int quit_app();
    void daemon_main();
    void quit_main();
    void lost_child();
    bool is_quit();
    static ACEApp* instance();

protected:
    ACEApp();
    char program_[256];
    bool is_quit_;
    bool is_child_;

private:
    void tweak_child(pid_t* child_pids, int child_proc_num);
    void parent_main(int child_proc_num);
    void parent_hatch(pid_t* child_pids, int child_proc_num);
    void parent_quit(pid_t* child_pids, int child_proc_num);
    int  init_log(int log_num = -1);
    void write_pid_file();

    bool tweak_child_;
    static ACEApp* aceapp_instance_;
};

#endif // _ACE_APP_H_ 
