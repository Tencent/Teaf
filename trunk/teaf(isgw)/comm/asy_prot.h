/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
//=============================================================================
/**
 *  @file    asy_prot.h
 *  此文件为后端异步处理模块需要的协议定义
 *  通过 ASYRKey 来进行状态管理的索引
 *  @author xwfang
 */
//=============================================================================

#ifndef _ASY_PROT_H_
#define _ASY_PROT_H_
#include "easyace_all.h"
#include "qmode_msg.h"

/// 协议解析回调函数指针
//typedef int (*ASY_PARSE)(PPMsg *ack);

/// 逻辑处理回调函数指针
typedef int (*ASY_PROC)(QModeMsg &ack, string& content, char* ack_buf, int& ack_len);

///
typedef struct stASYRKey
{
    uint32 sock_fd;
    uint32 sock_seq;
    uint32 msg_seq;
}ASYRKey;

///
typedef struct stASYRValue
{    
    uint32 time;  // 记录消息的时间 用于判断是否超时
    // 回调函数指针
    ASY_PROC asy_proc;
    // 可以用于保存当前处理的上下文
    string content;

    stASYRValue()
    {
        time = 0;
        asy_proc = NULL;
    }
}ASYRValue;

typedef struct stASYRMsg
{
   ASYRKey key;
   ASYRValue value;
}ASYRMsg;

/// ASYRKey
class ASYR_COMP
{
public:
    bool operator() (const ASYRKey& x, const ASYRKey& y) const
    {
        // sock_fd,sock_seq,msg_seq
        if (x.sock_fd == y.sock_fd 
        && x.sock_seq == y.sock_seq 
        && x.msg_seq == y.msg_seq 
        )
        {
            return false;
        }
        
        if (x.msg_seq == y.msg_seq) 
        {
            return true;
        }
        
        return x.msg_seq > y.msg_seq; //
    }
};

typedef map<ASYRKey, ASYRValue, ASYR_COMP> ASYR_MAP;

#endif // _ASY_PROT_H_
