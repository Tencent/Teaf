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
  FileName: ace_sock_hdr_base.h
  Author: xwfang              Date: 2008-06-01
  Description:
      tcp socket handler base 
***********************************************************/
#ifndef _ACE_SOCK_HDR_BASE_H_
#define _ACE_SOCK_HDR_BASE_H_
#include "easyace_all.h"

class AceSockHdrBase : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
    typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> super;
public:

    AceSockHdrBase();
    virtual ~AceSockHdrBase();
    virtual int open(void *p = 0);
    virtual int handle_output (ACE_HANDLE fd = ACE_INVALID_HANDLE);
    virtual int handle_close (ACE_HANDLE fd = ACE_INVALID_HANDLE
		, ACE_Reactor_Mask mask = ACE_Event_Handler::ALL_EVENTS_MASK);
    virtual int send(ACE_Message_Block* ack_msg = NULL);
    virtual int send_n(char* ack_msg, int len);
    virtual int get_seq();
	
protected:
    int sock_seq_; //当前连接的 seq 
    ACE_INET_Addr remote_addr_;
    ACE_Time_Value *time_null_;
    ACE_Time_Value time_zero_;
    static unsigned int total_seq_; //ACE_Atomic_Op<ACE_Thread_Mutex, int>     
};

#endif //_ACE_SOCK_HDR_BASE_H_
