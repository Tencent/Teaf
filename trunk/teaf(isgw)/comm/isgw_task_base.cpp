#include "isgw_task_base.h"

#include <ace/Log_Msg.h>

#include "easyace_all.h"

int IsgwTaskBase::open(void* p /*= 0*/)
{
    return activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED, thread_num_);
}

int IsgwTaskBase::put( ACE_Message_Block *mblk, ACE_Time_Value *timeout /*= 0*/ )
{
    return putq(mblk, timeout);
}

int IsgwTaskBase::stop()
{
    put(new ACE_Message_Block(0, ACE_Message_Block::MB_STOP));
    return wait();
}

int IsgwTaskBase::init()
{
    SysConf::instance()->get_conf_int(conf_section_.c_str(), "thread_num", (int*)&thread_num_);
    
    size_t high_water = 0;
    if(SysConf::instance()->get_conf_int(conf_section_.c_str(), "high_water_mark", (int*)&high_water) == 0)
    {
        msg_queue_->high_water_mark(high_water);
    }
    
    ACE_DEBUG((LM_INFO, "[%D] IsgwTaskBase::init thread num=%u, high water mark=%d\n", 
        thread_num_, high_water));
    return 0;
}

int IsgwTaskBase::svc()
{
    int ret = 0;
    for(ACE_Message_Block* mblk; getq(mblk) != -1;)
    {
        // is quit flag?
        if(mblk->size() == 0 && mblk->msg_type() == ACE_Message_Block::MB_STOP)
        {
            put(new ACE_Message_Block(0, ACE_Message_Block::MB_STOP));
            mblk->release();
            break;
        }
        
        ret = process(mblk);
        if(ret != 0)
        {
            mblk->release();
            break;
        }
        
        mblk->release();
    }
    
    return ret;
}

