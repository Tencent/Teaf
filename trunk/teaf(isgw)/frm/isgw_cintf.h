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
  FileName: isgw_cintf.h
  Author: xwfang              Date: 2010-08-20
  Description:
      tcp conn socket handler/net interface 
      2014-06-06 可以考虑继承自ISGWIntf 类 简化此类的实现
***********************************************************/
#ifndef _ISGW_CINTF_H_
#define _ISGW_CINTF_H_
#include "isgw_comm.h"
#include "ace_sock_hdr_base.h"
#include "../comm/pp_prot.h"

#ifndef MAX_RECV_BUF_LEN_C
#define MAX_RECV_BUF_LEN_C MAX_INNER_MSG_LEN
#endif

#ifndef MSG_QUE_SIZE_C
#define MSG_QUE_SIZE_C 10*1024*1024
#endif

typedef ACE_Message_Queue_Ex<PPMsg, ACE_MT_SYNCH> ISGWC_MSGQ;

// 后端连接处理器 处理从后端异步返回的消息 
class ISGWCIntf : public AceSockHdrBase
{    
public:
    ISGWCIntf();
    virtual ~ISGWCIntf();
    virtual int open(void * = 0);
    virtual int handle_input (ACE_HANDLE fd = ACE_INVALID_HANDLE);
    // 处理消息并放入本身的消息队列中或者直接透传到前端 
    virtual int process(char* msg, int sock_fd, int sock_seq, int msg_len);
    virtual int handle_close(ACE_HANDLE fd, ACE_Reactor_Mask mask);
    int is_legal(char* msg);
    int is_auth();
    // 从本地 queue_ 接收消息  time_out == NULL 默认阻塞 
    static int recvq(PPMsg*& msg, ACE_Time_Value* time_out=NULL);
    static int init();

private:
    char recv_buf_[MAX_RECV_BUF_LEN_C+1];
    unsigned int  recv_len_;
    unsigned int  msg_len_; //一个完整的消息包的长度，一般不包括消息长度字节
    static int msg_seq_;//消息本身的序列号和网络连接的序列号有区别
    
    static ISGWC_MSGQ queue_; //存放该连接上接收到的消息 
    static ACE_Time_Value zero_; 
};

typedef  ACE_Connector<ISGWCIntf, ACE_SOCK_CONNECTOR> ISGW_CONNECTOR;

#endif  //_ISGW_CINTF_H_
