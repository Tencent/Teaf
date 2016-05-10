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
*  @file      ace_svc.h
*  @author xwfang
*  @history 
*   200605 xwfang 消息控制线程模板类
*  
******************************************************************************/
#ifndef _ACE_SVC_H_
#define _ACE_SVC_H_
#include "ace_all.h"

#ifndef ACE_SVC_MSG_QUE_SIZE 
#define ACE_SVC_MSG_QUE_SIZE  1*1024*1024
#endif

template <typename IN_MSG_TYPE, typename OUT_MSG_TYPE>
class ACESvc : public ACE_Task_Base
{
    typedef ACE_Message_Queue_Ex<IN_MSG_TYPE, ACE_MT_SYNCH> ACESVC_MSGQ;
public:
    static ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>* instance()
    {
        ACE_DEBUG((LM_TRACE,
		"[%D] in ACESvc::instance\n"
		));
        if (instance_ == NULL)
        {
            instance_ = new ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>();
        }
        ACE_DEBUG((LM_TRACE,
		"[%D] out ACESvc::instance\n"
		));		
        return instance_;
    }       
    virtual int init();
    virtual int open (size_t hwm = ACE_Message_Queue_Base::DEFAULT_HWM*10,
		size_t lwm = ACE_Message_Queue_Base::DEFAULT_LWM*10,
		ACE_Notification_Strategy *sty = 0);
    virtual int fini();
    virtual int putq(IN_MSG_TYPE* msg);
    
protected:
    virtual int svc (void);
    virtual int recv(IN_MSG_TYPE*& msg);
    virtual OUT_MSG_TYPE* process(IN_MSG_TYPE*& msg);
    virtual int send(OUT_MSG_TYPE* ack);

protected:
    ACESVC_MSGQ queue_;
    ACE_Thread_Mutex queue_lock_;
    ACE_Time_Value time_out_;
    
    static ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE> *instance_;
	
protected:
    static const int DEFAULT_THREADS = 10;
};

template <typename IN_MSG_TYPE, typename OUT_MSG_TYPE>
ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>* ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>::instance_ = NULL;

template <typename IN_MSG_TYPE, typename OUT_MSG_TYPE>
int ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>::init()
{
    ACE_DEBUG((LM_TRACE,
		"[%D] in ACESvc::init\n"
		));

    //set que size 
    queue_.open(ACE_SVC_MSG_QUE_SIZE, ACE_SVC_MSG_QUE_SIZE, NULL);

    //set enqueue time out 0
    time_out_.set(0,0);

    //active num thread
    activate(THR_NEW_LWP | THR_JOINABLE, DEFAULT_THREADS);

    ACE_DEBUG((LM_INFO, "[%D] ACESvc init succ,threads=%d,inner lock=0x%x,my lock=0x%x\n"
        , DEFAULT_THREADS
        , &(queue_.lock())
        , &(queue_lock_.lock())
        ));
    
    ACE_DEBUG((LM_TRACE,
		"[%D] out ACESvc::init\n"
		));
    return 0;
}

template <typename IN_MSG_TYPE, typename OUT_MSG_TYPE>
int ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>::open (size_t hwm,
											size_t lwm, ACE_Notification_Strategy *sty)
{
    ACE_DEBUG((LM_INFO, "[%D] ACESvc open que size hwm=%d,lwm=%d\n", hwm, lwm));
    return queue_.open (hwm, lwm, sty);
}

template <typename IN_MSG_TYPE, typename OUT_MSG_TYPE>
int ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>::fini()
{
    ACE_DEBUG((LM_TRACE,
		"[%D] in ACESvc::fini\n"
		));
    queue_.deactivate();

    this->thr_mgr()->kill_grp(this->grp_id(), SIGINT);
    this->thr_mgr()->wait_grp(this->grp_id());
    
    ACE_DEBUG((LM_TRACE,
		"[%D] out ACESvc::fini\n"
		));
    return 0;
}

template <typename IN_MSG_TYPE, typename OUT_MSG_TYPE>
int ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>::putq(IN_MSG_TYPE* msg)
{
    ACE_DEBUG((LM_TRACE,
		"[%D] in ACESvc::putq\n"
		));
    int ret = 0;
    if ( msg != NULL )
    {
        ret = queue_.enqueue(msg, &time_out_);
        if (ret == -1)
        {
            ACE_DEBUG((LM_ERROR,
        		"[%D] ACESvc enqueue msg failed,msg count is %d\n"
        		, queue_.message_count()
        		));
            return -1;
        }
        ACE_DEBUG((LM_TRACE,
            "[%D] ACESvc enqueue msg succ,msg count is %d,ret=%d\n"
            , queue_.message_count()
            , ret
            ));
    }
    ACE_DEBUG((LM_TRACE,
		"[%D] out ACESvc::putq\n"
		));
    return ret;
}

template <typename IN_MSG_TYPE, typename OUT_MSG_TYPE>
int ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>::recv(IN_MSG_TYPE*& msg)
{
    // 此处应该不需要加锁 本身消息队列是 ACE_MT_SYNCH 的
    // ACE_Guard<ACE_Thread_Mutex> guard(queue_lock_);    
    int ret = 0;
    
    ACE_DEBUG((LM_TRACE,
        "[%D] ACESvc before dequeue msg,msg count %d\n"
        , queue_.message_count()
        ));
    
    ret = queue_.dequeue(msg);
    if ( msg == NULL || ret == -1 )
    {
        ACE_DEBUG((LM_ERROR,
            "[%D] ACESvc dequeue msg failed or recv a null msg\n"
            ));
    }
    ACE_DEBUG((LM_TRACE,
        "[%D] ACESvc after dequeue msg,msg count %d\n"
        , queue_.message_count()
        ));
    
    return ret;
}

template <typename IN_MSG_TYPE, typename OUT_MSG_TYPE>
OUT_MSG_TYPE* ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>::process(IN_MSG_TYPE*& msg)
{
    ACE_DEBUG((LM_TRACE,
        "[%D] in ACESvc::process\n"
        ));
    //处理完成用户要清理源消息，不清理，后续svc内部会自动清理
    
    ACE_DEBUG((LM_TRACE,
        "[%D] out ACESvc::process\n"
        ));
    return NULL;
}

template <typename IN_MSG_TYPE, typename OUT_MSG_TYPE>
int ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>::send(OUT_MSG_TYPE* ack)
{
    ACE_DEBUG((LM_TRACE,
    	"[%D] in ACESvc::send\n"
    	));
    ACE_UNUSED_ARG(ack);

    ACE_DEBUG((LM_TRACE,
    	"[%D] out ACESvc::send\n"
    	));
    return 0;
}

template <typename IN_MSG_TYPE, typename OUT_MSG_TYPE>
int ACESvc<IN_MSG_TYPE, OUT_MSG_TYPE>::svc(void)
{
    unsigned int threadid = syscall(__NR_gettid); //ACE_OS::thr_self();
    unsigned int pid = ACE_OS::getpid();
    
    ACE_DEBUG((LM_INFO,
        "[%D] ACESvc (%u:%u) enter svc ...\n"
        , threadid, pid
        ));
    
    while (1)
    {  
        IN_MSG_TYPE* msg = NULL;
        int ret = recv(msg);
        if (ret == -1)
        {
            if (errno == ESHUTDOWN)
            {
                ACE_DEBUG((LM_ERROR,
                    "[%D] ACESvc (%u:%u) recv msg failed,ret=%d,errno=%d,break\n"
                    , threadid, pid, ret, errno
                    ));
                break; // recv exit directive
            }
            else
            {
                ACE_DEBUG((LM_ERROR,
                    "[%D] ACESvc (%u:%u) recv msg failed,ret=%d,errno=%d,continue\n"
                    , threadid, pid, ret, errno
                    ));
                continue;
            }
        }

        if (msg == NULL)
        {
            ACE_DEBUG((LM_ERROR,
                "[%D] ACESvc (%u:%u) recv a null msg, continue\n"
                , threadid, pid
                ));
            continue;
        }

        OUT_MSG_TYPE* ack = process(msg);
        /* 消息由应用层自己管理 建议进出都用同一条消息
        if (msg != NULL )
        {
            ACE_DEBUG((LM_ERROR,
                "[%D] ACESvc (%u:%u) svc del proceed msg\n"
                , threadid, pid
                ));
            delete msg;
            msg = NULL;
        }
        */
        
        if (ack != NULL )
        {
            send(ack);
        }	
    }
    
    ACE_DEBUG((LM_INFO,
        "[%D] ACESvc (%u:%u) leave svc ...\n"
        , threadid, pid
        ));
    return 0;
}

#endif //_ACE_SVC_H_
