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
*  @file ace_object_que.h
*  @author xwfang
*  @history 
*   200805 xwfang 对象池实现，主要用于管理空白对象，解决
*   频繁分配对象产生的一系列问题
*  
******************************************************************************/
#ifndef _ACE_OBJECT_QUE_H_
#define _ACE_OBJECT_QUE_H_
#include "ace_all.h"

template <typename ACE_MESSAGE_TYPE>
class ACE_Object_Que
{
public:
    static ACE_Object_Que<ACE_MESSAGE_TYPE>* instance()
    {
        if (instance_ == NULL)
        {
            ACE_DEBUG((LM_INFO, "[%D] ACE_Object_Que new instance\n"));
            instance_ = new ACE_Object_Que<ACE_MESSAGE_TYPE>();
        }
        return instance_;
    }
    
    ACE_Object_Que();
    virtual ~ACE_Object_Que();
    virtual int init(int msg_counts = 500);
    virtual int set_timeout (long sec = 0, long usec = 10);
    virtual int open (size_t hwm = ACE_Message_Queue_Base::DEFAULT_HWM*10,
		size_t lwm = ACE_Message_Queue_Base::DEFAULT_LWM*10,
		ACE_Notification_Strategy *sty = 0);
    virtual int enqueue (ACE_MESSAGE_TYPE *&new_item, int index=0);
    virtual int dequeue (ACE_MESSAGE_TYPE *&first_item);
	
private:
    ACE_Message_Queue_Ex<ACE_MESSAGE_TYPE, ACE_MT_SYNCH> que_imp_;
    ACE_Time_Value timeout_;
    int ret_;
    static ACE_Object_Que<ACE_MESSAGE_TYPE> *instance_;
};

template <typename ACE_MESSAGE_TYPE>
ACE_Object_Que<ACE_MESSAGE_TYPE>* ACE_Object_Que<ACE_MESSAGE_TYPE>::instance_ = NULL;

template <typename ACE_MESSAGE_TYPE>
ACE_Object_Que<ACE_MESSAGE_TYPE>::ACE_Object_Que ()
{
    ACE_DEBUG((LM_INFO, "[%D] ACE_Object_Que construct succ.\n"));
}

template <typename ACE_MESSAGE_TYPE>
ACE_Object_Que<ACE_MESSAGE_TYPE>::~ACE_Object_Que ()
{
    ACE_DEBUG((LM_INFO, "[%D] ACE_Object_Que destruct succ.\n"));
}

template <typename ACE_MESSAGE_TYPE>
int ACE_Object_Que<ACE_MESSAGE_TYPE>::init (int msg_counts)
{    
    timeout_.set(0, 10);

    //有可能预先含有对象，当新指定的消息个数
    //大于原先的消息个数大小才重新分配
    if ( msg_counts > que_imp_.message_count())
    {
        que_imp_.close();
        que_imp_.open (msg_counts*sizeof(ACE_MESSAGE_TYPE), msg_counts*sizeof(ACE_MESSAGE_TYPE), NULL);
    }

    ACE_MESSAGE_TYPE *msg = NULL;
    for(int i=que_imp_.message_count(); i<msg_counts; i++)
    {
        ACE_NEW_NORETURN(msg, ACE_MESSAGE_TYPE);
        ret_ = que_imp_.enqueue(msg, &timeout_);
        if ( ret_ == -1 )
        {
            ACE_DEBUG((LM_ERROR, "[%D] ACE_Object_Que init enqueue failed,may be que is full,delete this msg\n"));
            //入队失败则把此空白对象删除
            if ( msg != NULL )
            {
                delete msg;
                msg = NULL;
            }
            break;
        }
    }
    
    ACE_DEBUG((LM_INFO, "[%D] ACE_Object_Que init msg succ,msg counts=%d\n"
        ,que_imp_.message_count()
        ));
    return 0;
}

template <typename ACE_MESSAGE_TYPE>
int ACE_Object_Que<ACE_MESSAGE_TYPE>::set_timeout (long sec, long usec)
{
    timeout_.set(sec, usec);
    return 0;
}

template <typename ACE_MESSAGE_TYPE>
int ACE_Object_Que<ACE_MESSAGE_TYPE>::open (size_t hwm,
											size_t lwm, ACE_Notification_Strategy *sty)
{
    return que_imp_.open (hwm, lwm, sty);
}

template <typename ACE_MESSAGE_TYPE>
int ACE_Object_Que<ACE_MESSAGE_TYPE>::enqueue (ACE_MESSAGE_TYPE *&new_item, int index)
{
    if(new_item == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ACE_Object_Que enqueue failed,new_item is NULL"));
        return -1;
    }
    
    ret_ = que_imp_.enqueue(new_item, &timeout_);
    if ( ret_ == -1 )
    {
        //入队失败则把此空白对象丢失
        if ( new_item != NULL )
        {
            delete new_item;
            new_item = NULL;
        }
        ACE_DEBUG((LM_ERROR, "[%D] ACE_Object_Que enqueue failed,may be que is full"
            ",delete this blank msg\n"));
        return -1;
    }
    ACE_DEBUG((LM_TRACE, "[%D] ACE_Object_Que enqueue a msg succ,ret=%d\n", ret_));
    new_item = NULL;
    return 0;
}

template <typename ACE_MESSAGE_TYPE>
int ACE_Object_Que<ACE_MESSAGE_TYPE>::dequeue (ACE_MESSAGE_TYPE *&first_item)
{
#ifdef ISGW_TRACE
    unsigned int threadid = syscall(__NR_gettid); //ACE_OS::thr_self();
    unsigned int pid = ACE_OS::getpid();
#endif
    ret_ = que_imp_.dequeue(first_item, &timeout_);
    if ( ret_ == -1 )
    {
        //获取不到则重新 产生 一个新的，并传递出去
        ACE_NEW_RETURN(first_item, ACE_MESSAGE_TYPE, ret_);
        ACE_DEBUG((LM_ERROR, "[%D] ACE_Object_Que dequeue failed,return a new blank msg\n"));
        return -1;
    }
#ifdef ISGW_TRACE
    ACE_DEBUG((LM_TRACE, "[%D] ACE_Object_Que (%u:%u) dequeue a msg succ\n", threadid, pid));
#endif
    //返回内存之前先清理内存，确保返回的是空白的内存
    //memset(first_item, 0x0, sizeof(ACE_MESSAGE_TYPE));
    
    return 0;
}

#endif//_ACE_OBJECT_QUE_H_
