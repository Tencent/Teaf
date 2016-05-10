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
  FileName: isgw_sighdl.h
  Author: ian              Date: 2011-01-25
  Description:
      signal handler
      201308 扩展支持定时器      
***********************************************************/

#ifndef _ISGW_SIGHDL_H_
#define _ISGW_SIGHDL_H_

#include "isgw_comm.h"

class ISGWSighdl: public ACE_Event_Handler
{
public:
    ~ISGWSighdl();

    int handle_signal(int signum, siginfo_t * = 0, ucontext_t * = 0);
    int handle_timeout(const ACE_Time_Value& tv, const void *arg);
    //int handle_exception(ACE_HANDLE);

    static ISGWSighdl* instance();
        
private:
    ISGWSighdl(); //避免多个对象被创建
    
private:
    int handle_reload();

private:
    static ISGWSighdl* instance_; 
};

#endif // _ISGW_UINTF_H_
