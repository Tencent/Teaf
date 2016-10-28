/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
/*******************************************************************************
 *  @file    ibc_prot.h
 *
 *  此文件为内部批量处理模块需要的协议定义 
 *  批量处理指令通过分解之后拥有共同的 IBCRKey 通过这个来关联
 *  暂时只支持 tcp 协议 
 *  
 *  @author xwfang
 ******************************************************************************/
#ifndef _IBC_PORT_H_
#define _IBC_PORT_H_
#include "easyace_all.h"
#include "qmode_msg.h"
#include "pp_prot.h"

///内部批量处理结果 数据结构 key 部分
typedef struct stIBCRKey
{
    uint32 sock_fd; //socket 的文件描述符    
    uint32 sock_seq; //socket的序列号，sock_fd值相同时可以用来区分
    uint32 msg_seq; //消息的序列号，唯一标识一个请求
}IBCRKey;

///内部批量处理结果 数据结构 value 部分 
typedef struct stIBCRValue
{
    //ACE_Thread_Mutex lock; //只有获得此锁才能对此结构进行操作 
    uint32 time; 
    uint32 cmd; 
    uint32 uin; 
    uint32 total; // 总的需要处理的关联记录数 
    uint32 cnum; // 当前已经处理的关联记录数 
    uint32 snum; //  处理成功的关联记录数 
    uint32 msg_len; //后面的有效消息长度
    uint32 sock_fd_; //以下带 _ 的这些为透传的信息 
    uint32 sock_seq_;
    uint32 msg_seq_;
    uint32 prot_;
    uint32 time_;
    int rflag_;
    int endflag_; // 是否进行end 回调 
    char msg[MAX_INNER_MSG_LEN+1]; //接受到的消息体
    std::list<std::string> msg_list; // 接受的消息体
    uint32 msg_num; // 消息体记录数

    stIBCRValue()
    {
        time = 0;
        cmd = 0;
        uin = 0;
        total = 0;
        cnum = 0;
        snum = 0;
        msg_len = 0;
        sock_fd_ = 0;
        sock_seq_ = 0;
        msg_seq_ = 0;
        prot_ = 0;
        time_ = 0;
        rflag_ = 0;
        endflag_ = 0;
        msg_num = 0;
        memset(msg, 0x0, sizeof(msg));
        msg_list.clear();
        msg_num = 0;
    }
}IBCRValue;

///内部批量处理结果 数据结构 
typedef struct stIBCRMsg
{
   IBCRKey key;
   IBCRValue value;
}IBCRMsg;

/// IBCRKey 比较函数 
class IBCR_COMP
{
public:
    bool operator() (const IBCRKey& x, const IBCRKey& y) const
    {
        // sock_fd,sock_seq,msg_seq 相同的情况返回 false 确保作为唯一主键 
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
    	
        return x.msg_seq > y.msg_seq; // 从大到小，因为大的比较新，删除从尾部删除    
    }
};

///存储结果的 map 
typedef map<IBCRKey, IBCRValue, IBCR_COMP> IBCR_MAP;

#endif // _IBC_PORT_H_
