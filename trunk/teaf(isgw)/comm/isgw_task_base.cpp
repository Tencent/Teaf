#include "isgw_task_base.h"
#include "stat.h"

// put 消息到异步工作线程队列
int IsgwTaskBase::put(const char * req_buf, int len, int type)
{
    ACE_Message_Block *mblk = new ACE_Message_Block(len + sizeof(int) * 2);
    if(mblk == NULL)
    {
        ACE_DEBUG((LM_ERROR,"[%D] IsgwTaskBase put msg failed,type=%d,info=%s\n"
            , type, "allocate mem"));
        return -1;
    }
    
    mblk->copy((const char *)&type, sizeof(type));
    mblk->copy((const char *)&len, sizeof(len));
    mblk->copy(req_buf, len);
    
    ACE_Time_Value timeout(0, 0);
    if(put(mblk, &timeout) < 0)
    {
        Stat::instance()->incre_stat(STAT_CODE_ASYNC_ENQUEUE);        
        ACE_DEBUG((LM_ERROR, "[%D] IsgwTaskBase put msg failed,type=%d,len=%d\n"
            , type, len));
        return -1;
    }
    
    ACE_DEBUG((LM_DEBUG,"[%D] IsgwTaskBase put msg succ,type=%d,len=%d\n"
        , type, len));
    return 0;
}

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
    SysConf::instance()->get_conf_int(conf_section_.c_str(), "threads", (int*)&thread_num_);
    if(thread_num_ <= 0)
    {
        thread_num_ = DEF_THREAD_NUM;
    }
    
    size_t high_water = 0;
    if(SysConf::instance()->get_conf_int(conf_section_.c_str(), "high_water_mark", (int*)&high_water) == 0)
    {
        msg_queue_->high_water_mark(high_water);
    }
    
    ACE_DEBUG((LM_INFO, "[%D] IsgwTaskBase::init thread num=%u, high water mark=%d\n", 
        thread_num_, high_water));
    return open();
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

