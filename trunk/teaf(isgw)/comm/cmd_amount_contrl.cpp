#include "cmd_amount_contrl.h"


CmdAmntCntrl::CmdAmntCntrl()
{
    cmd_no_start_ = 0;
    cmd_no_end_ = 0;
    time_now_ = time(0);
    nodes_ = NULL;
    
    init_config("cmd_amnt_cntrl");
}

CmdAmntCntrl::CmdAmntCntrl(char *config_item)
{
    cmd_no_start_ = 0;
    cmd_no_end_ = 0;
    time_now_ = time(0);
    nodes_ = NULL;
    
    init_config(config_item);
}

CmdAmntCntrl::~CmdAmntCntrl()
{
    for(int i=0; i<=cmd_no_end_ -cmd_no_start_+1; i++)
    {
        delete nodes_[i].status_lock_;
        delete nodes_[i].statiscs_lock_;
    }

    delete [] nodes_;
    nodes_ = NULL;
}

int CmdAmntCntrl::init_config(char *config_item)
{
    //需要配置命令字的最大和最小值
    //这样做的目的是为了使用数组来存放各命令字的统计信息
    SysConf::instance()->get_conf_int( config_item, "start_cmd", &cmd_no_start_);
    SysConf::instance()->get_conf_int( config_item, "end_cmd", &cmd_no_end_);
    
    int interval = 30;
    int threshold = 60000;
    int threshold_ratio = 80;
    SysConf::instance()->get_conf_int( config_item, "interval", &interval);
    SysConf::instance()->get_conf_int( config_item, "max_req", &threshold);
    SysConf::instance()->get_conf_int( config_item, "max_fail_ratio", &threshold_ratio);

    ACE_DEBUG((LM_INFO, "[%D] CmdAmntCntrl init start_cmd=%d,"
        "end_cmd=%d,intvl=%d,max_req=%d,max_fail_ratio=%d\n"
        , cmd_no_start_
        , cmd_no_end_
        , interval
        , threshold
        , threshold_ratio
        ));
    
    int node_num = cmd_no_end_ -cmd_no_start_;
    char temp[32];

    if(cmd_no_start_>=0 && node_num >0 && NULL==nodes_)
    {
        nodes_ = new CmdStaticsNode[node_num+1];
        for(int i=0; i<=node_num; i++)
        {
            nodes_[i].cmd = cmd_no_start_+i;
            nodes_[i].status = 0;
            nodes_[i].time_intvl = interval;
            nodes_[i].status_lock_ = new ACE_Thread_Mutex();
            nodes_[i].statiscs_lock_ = new ACE_Thread_Mutex();
            nodes_[i].max_req_limit = threshold;
            nodes_[i].max_fail_ratio_limit = threshold_ratio;
            //读取针对每个指令的配置信息，若没有则是用默认值
            snprintf(temp, sizeof(temp), "interval_%d", nodes_[i].cmd);
            SysConf::instance()->get_conf_int( config_item, temp, &(nodes_[i].time_intvl));
            snprintf(temp, sizeof(temp), "max_req_%d", nodes_[i].cmd);
            SysConf::instance()->get_conf_int( config_item, temp, &(nodes_[i].max_req_limit));
            snprintf(temp, sizeof(temp), "max_fail_ratio_%d", nodes_[i].cmd);
            SysConf::instance()->get_conf_int( config_item, temp, &(nodes_[i].max_fail_ratio_limit));
        }
    }

    return 0;
    
}

//按照目前的定义 status
// == 0 是正常的 不会进行限制
// > 0 会进行流量限制
int CmdAmntCntrl::get_status(int cmd_no, unsigned int now_t)
{
    time_now_ = now_t;
    
    if(cmd_no<cmd_no_start_ || cmd_no>cmd_no_end_ || nodes_ == NULL)
        return 0; // 表示不做限制
    
    int index = cmd_no - cmd_no_start_;
    ACE_Guard<ACE_Thread_Mutex> guard(*nodes_[index].status_lock_);
    
    int status= nodes_[index].status;
    //如果当前是当前周期拒绝服务，并且达到一个周期的时间，则清除status标志位
    if(1 == status&&(time_now_-nodes_[index].time_start >nodes_[index].time_intvl))
    {
        nodes_[index].time_start = time_now_;
        nodes_[index].status = 0;
        status = 0;
        
        nodes_[index].cur_fail_req = 0;
        nodes_[index].cur_req = 0;
        nodes_[index].cur_fail_ratio = 0;
    }
    return status;
}

/**********************************************************************
function : 设置指定指令的状态
in para  :
    cmd_no 指定的命令字
    status 指令的状态
            0，正常
            1，当前周期不提供服务
            2，停止后续所有服务
out para:
return   :
desc     :
**********************************************************************/
void CmdAmntCntrl::set_status(int cmd_no, int status)
{
    if(cmd_no<cmd_no_start_ || cmd_no>cmd_no_end_ || nodes_ == NULL)
        return ;

    int index = cmd_no - cmd_no_start_;
    ACE_Guard<ACE_Thread_Mutex> guard(*nodes_[index].status_lock_);
    
    nodes_[index].status = status;
    if(0==status)
    {
        nodes_[index].cur_fail_req = 0;
        nodes_[index].cur_req = 0;
        nodes_[index].cur_fail_ratio = 0;
    }
}

void CmdAmntCntrl::set_time(unsigned int now_t)
{
    time_now_ = now_t;
}

/**********************************************************************
function : 对指定指令的流量控制
in para  :
    cmd_no 指定的命令字
    result 该命令的执行结果
            0，为执行争取
            !=0，执行失败
out para:
return   :
desc     :
**********************************************************************/
void CmdAmntCntrl::amount_inc(int cmd_no, int result)
{
    if(cmd_no<cmd_no_start_ ||cmd_no>cmd_no_end_ || nodes_ == NULL)
        return ;
    
    int index = cmd_no - cmd_no_start_;
    ACE_Guard<ACE_Thread_Mutex> guard(*nodes_[index].statiscs_lock_);

    //如果当前周期结束，重置相关值
    if(time_now_-nodes_[index].time_start >= nodes_[index].time_intvl)
    {
        nodes_[index].time_start = time_now_;
        
        if(nodes_[index].cur_req > nodes_[index].max_req)
            nodes_[index].max_req = nodes_[index].cur_req;
        
        if(nodes_[index].cur_fail_ratio > nodes_[index].max_fail_ratio)
            nodes_[index].max_fail_ratio = nodes_[index].cur_fail_ratio;
        
        nodes_[index].cur_fail_req = 0;
        nodes_[index].cur_req = 0;
        nodes_[index].cur_fail_ratio = 0;
    }
    //如果当前周期的请求量查过阀值，本周期拒绝服务
    else if(nodes_[index].cur_req >= nodes_[index].max_req_limit)
    {
        set_status(cmd_no, 1);
    }

    //根据当前请求的结果，更新相关的统计信息
    nodes_[index].total_req++;
    nodes_[index].cur_req++;
    if(result<ERROR_OK)
    {
        nodes_[index].total_fail_req++;
        nodes_[index].cur_fail_req++;
        //更新当前周期的失败率，如果失败的次数大于10次，并且失败率超过阀值
        //拒绝周期所有请求
        nodes_[index].cur_fail_ratio = (100*nodes_[index].cur_fail_req)/nodes_[index].cur_req;
        if(nodes_[index].cur_fail_ratio>nodes_[index].max_fail_ratio_limit
            && nodes_[index].cur_fail_req>10)
        {
            set_status(cmd_no, 1);
        }
    }
    ACE_DEBUG((LM_TRACE, "[%D] CmdAmntCntrl::amount_inc cmd=%d,"
        "total=%d,cur_total=%d,fail=%d,cur_fail=%d\n"
        , cmd_no
        , nodes_[index].total_req
        , nodes_[index].cur_req
        , nodes_[index].total_fail_req
        , nodes_[index].cur_fail_req
        ));
        
}

/**********************************************************************
function : 出处指定命令的统计信息
in para  :
    cmd_no 指定的命令字
    out_info 输出缓冲区地址
    len 出缓冲区的长度
out para:
return   :
desc     :
**********************************************************************/
void CmdAmntCntrl::get_statiscs(int cmd_no, char*out_info, int len)
{
    if(cmd_no<cmd_no_start_ ||cmd_no>cmd_no_end_ || nodes_ == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] CmdAmntCntrl get_statiscs invalid cmd no=%d,nodes=%p\n", cmd_no, nodes_));
        snprintf(out_info, len, "info=invalid cmd no,between %d-%d", cmd_no_start_, cmd_no_end_);
        return ;
    }
    int index = cmd_no -cmd_no_start_;
    ACE_Guard<ACE_Thread_Mutex> guard(*nodes_[index].statiscs_lock_);
    
    snprintf(out_info, len, "status=%d&total=%d&peak=%d&cur=%d&fail_total=%d&fail_peak=%d&fail_cur=%d"
        , nodes_[index].status
        , nodes_[index].total_req
        , nodes_[index].max_req
        , nodes_[index].cur_req
        , nodes_[index].total_fail_req
        , nodes_[index].max_fail_ratio
        , nodes_[index].cur_fail_ratio
        );
}

