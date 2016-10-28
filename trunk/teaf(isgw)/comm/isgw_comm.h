/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
#ifndef _ISGW_COMM_H_
#define _ISGW_COMM_H_
#include "pp_prot.h"
#include "easyace_all.h"
#include "qmode_msg.h"
//#include "object_que.h"

#define ISGW_Object_Que ACE_Object_Que

#ifndef GUARD_DOG_WATCH 
#define GUARD_DOG_WATCH(latch) ACE_Guard<ACE_Thread_Mutex> guard(latch)
#endif

namespace EASY_UTIL //easy  的名空间
{

typedef struct stSOCKET_INFO
{
    unsigned int index; //索引
    unsigned int sock_fd; //
    unsigned int sock_seq; //
    unsigned int sock_ip;
    unsigned int creat_time; //连接产生的时间
    int status; //0 不在使用 1 正在被使用中
}SOCKET_INFO;

///编码特殊字符
char *prot_encode(char *dest, const char *src);
///过滤特殊字符
char *prot_strim(char *dest, const char *src);
// 判断是否是合法的uin，合法的uin >10000 <int
int is_valid_uin(unsigned int uin);
unsigned int get_time();
unsigned int get_span(struct timeval *tv1, struct timeval *tv2);

};

#endif //_ISGW_COMM_H_
