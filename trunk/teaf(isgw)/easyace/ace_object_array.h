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
*  @file ace_object_array.h
*  @author xwfang
*  @history 
*   201507 xwfang
*   数组对象池实现，主要用于管理空白对象，解决频繁分配对象产生的一系列问题
*   此实现要求结构体本身具备记录索引下标的功能 这样才能正常的回收
*  
******************************************************************************/
#ifndef _ACE_OBJECT_ARRAY_H_
#define _ACE_OBJECT_ARRAY_H_
#include "ace_all.h"

template <typename ACE_MESSAGE_TYPE>
class ACE_Object_Array
{
    //typedef ACE_MESSAGE_TYPE* ACE_MESSAGE_TYPE_PTR;
public:
    // 不加锁 需要在单线程环境调用
    static ACE_Object_Array<ACE_MESSAGE_TYPE>* instance()
    {
        if (instance_ == NULL)
        {
            ACE_DEBUG((LM_INFO, "[%D] ACE_Object_Array new instance\n"));
            instance_ = new ACE_Object_Array<ACE_MESSAGE_TYPE>();
        }
        return instance_;
    }
    ACE_Object_Array();
    virtual ~ACE_Object_Array();
    // 不加锁 需要在单线程环境调用
    virtual int init(int msg_counts = 5000);


    virtual int enqueue (ACE_MESSAGE_TYPE *&new_item, int index=0);
    // 需要使用者保存返回的索引并在 enqueue 的时候使用
    virtual int dequeue (ACE_MESSAGE_TYPE *&first_item);
	
private:
    static const int msg_counts_ = 5000;
    ACE_Atomic_Op<ACE_Thread_Mutex, unsigned int> cindex_; //记录当前使用的下标
    int flag_[msg_counts_]; //是否使用的下标
    ACE_MESSAGE_TYPE* que_imp_[msg_counts_];

    static ACE_Object_Array<ACE_MESSAGE_TYPE> *instance_;
};

template <typename ACE_MESSAGE_TYPE>
ACE_Object_Array<ACE_MESSAGE_TYPE>* ACE_Object_Array<ACE_MESSAGE_TYPE>::instance_ = NULL;

template <typename ACE_MESSAGE_TYPE>
ACE_Object_Array<ACE_MESSAGE_TYPE>::ACE_Object_Array ()
{
    ACE_DEBUG((LM_INFO, "[%D] ACE_Object_Array construct succ.\n"));
}

template <typename ACE_MESSAGE_TYPE>
ACE_Object_Array<ACE_MESSAGE_TYPE>::~ACE_Object_Array ()
{
    ACE_DEBUG((LM_INFO, "[%D] ACE_Object_Array destruct succ.\n"));
}

template <typename ACE_MESSAGE_TYPE>
int ACE_Object_Array<ACE_MESSAGE_TYPE>::init (int msg_counts)
{
    //msg_counts 只是为了兼容，其实是改不了数组大小的

    ACE_MESSAGE_TYPE* msg = NULL;
    for(int i=0; i<msg_counts_; i++)
    {
        ACE_DEBUG((LM_TRACE, "[%D] ACE_Object_Array init msg,start to new the %d msg\n", i));
        ACE_NEW_NORETURN(msg, ACE_MESSAGE_TYPE);
        que_imp_[i] = msg;
        flag_[i] = 0; //设置未使用标志
    }
    cindex_ = 0;
    
    ACE_DEBUG((LM_INFO, "[%D] ACE_Object_Array init msg succ,msg counts=%d\n",msg_counts_));
    return 0;
}

template <typename ACE_MESSAGE_TYPE>
int ACE_Object_Array<ACE_MESSAGE_TYPE>::enqueue (ACE_MESSAGE_TYPE *&new_item, int index)
{
    if (index < msg_counts_)
    {
        //设置为未使用即可
        flag_[index] = 0;
    }

    return 0;
}

template <typename ACE_MESSAGE_TYPE>
int ACE_Object_Array<ACE_MESSAGE_TYPE>::dequeue (ACE_MESSAGE_TYPE *&first_item)
{
    int index = cindex_++%msg_counts_; //要确保此++操作是原子的
    if(flag_[index] == 0)
    {
        first_item = que_imp_[index];
        flag_[index] = 1;
    }

    return index;
}

#endif//_ACE_Object_Array_H_
