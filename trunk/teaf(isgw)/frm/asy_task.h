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
*  @file      asy_task.h
*  @author xwfang
*  @history 
*   201405 xwfang 跟后端的异步联接的返回消息处理类(单独线程处理)
*  
******************************************************************************/
#ifndef _ASY_TASK_H_
#define _ASY_TASK_H_
#include "sys_comm.h"
#include "isgw_comm.h"
#include "asy_prot.h"

#define MAX_ASYR_RECORED 10240*2 //map 里面最多的记录数 
#define DISCARD_TIME 5  //超时丢弃

class ASYTask : public ACE_Task_Base
{
public:
    static ASYTask* instance()
    {
        ACE_DEBUG((LM_TRACE,
		"[%D] in ASYTask::instance\n"
		));
        if (instance_ == NULL)
        {
            instance_ = new ASYTask();
        }
        ACE_DEBUG((LM_TRACE,
		"[%D] out ASYTask::instance\n"
		));		
        return instance_;
    }       
    virtual int init();
    virtual int fini();
    virtual int insert(ASYRMsg &rmsg);
    //设置缺省的回调函数
    virtual int set_proc(ASY_PROC asy_proc);
    
protected:
    virtual int svc (void);

protected:
    //ACE_Time_Value time_out_;
    
    static ASYTask *instance_;
    //存放异步消息的相关结果
    static ASYR_MAP asyr_map_; 
    static ACE_Thread_Mutex asyr_map_lock_;
    static ASY_PROC asy_proc_; //缺省的回调函数
    
protected:
    static const int DEFAULT_THREADS = 5;
};

#endif //_ASY_TASK_H_
