/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
#ifndef ISGW_TASK_BASE_H
#define	ISGW_TASK_BASE_H

#include <string>
#include <ace/Task.h>

class IsgwTaskBase : public ACE_Task<ACE_MT_SYNCH> 
{
public:
    IsgwTaskBase(const std::string& conf_section = "comm") : conf_section_(conf_section), 
            thread_num_(DEF_THREAD_NUM) {}
    
public:
    virtual int init();
    virtual int stop();
    
public:
    virtual int open(void* p = 0);
    virtual int svc();
    virtual int put (ACE_Message_Block *mblk, ACE_Time_Value *timeout = 0);
    
protected:
    virtual int process(ACE_Message_Block* mblk) = 0;

protected:
    std::string conf_section_;
    unsigned int thread_num_;
    
private:
    static const unsigned int DEF_THREAD_NUM = 1;
};

#endif	/* ISGW_TASK_BASE_H */

