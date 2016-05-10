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
 *  @file    cmd_amount_contrl.cpp
 *
 *  此文件为是为了实现流量控制的功能
 *  
 *  @author ian
 */
//=============================================================================

#ifndef _CMD_AMOUNT_CONTRL_H_
#define _CMD_AMOUNT_CONTRL_H_

#include "easyace_all.h"
#include "ace/RW_Thread_Mutex.h"
#include "pp_prot.h"

class CmdAmntCntrl
{
public:
    struct CmdStaticsNode
    {
        CmdStaticsNode()
        {
            status_lock_ = NULL;
            statiscs_lock_ = NULL;
            cmd = 0;
            status = 0;
            total_req = 0;
            total_fail_req = 0;
            time_intvl = 0;
            time_start = 0;
            cur_req = 0;
            cur_fail_req = 0;
            cur_fail_ratio = 0;
            max_req = 0;
            max_req_limit = 0;
            max_fail_ratio = 0;
            max_fail_ratio_limit = 0;
        }
        
        ACE_Thread_Mutex *status_lock_;     //同步命令状态的互斥锁
        ACE_Thread_Mutex *statiscs_lock_;       //同步其他统计数据的互斥锁
        
        int cmd;        //命令字
        int status;     //命令状态，0正常，1当前周期屏蔽请求，2停止提供服务
        int total_req;          //请求总数
        int total_fail_req;           //返回失败的次数

        //流量控制相关信息
        int time_intvl;         //监控周期，单位秒
        unsigned int time_start;        //当前周期的起点
        
        int cur_req;      //当前周期的请求量
        int cur_fail_req;       //当前周期的失败返回量
        int cur_fail_ratio;     //当前周期的请求失败率
        
        int max_req;           //请求的峰值
        int max_req_limit;           //请求的最大门限值
        int max_fail_ratio;         //请求最大失败率，大于该值则拒绝服务
        int max_fail_ratio_limit;       //失败率的最大阀值
        
    };

    CmdAmntCntrl();
    CmdAmntCntrl(char *config_item);
    ~CmdAmntCntrl();

    int init_config(char *config_item);
    int get_status(int cmd_no, unsigned int now_t);
    void set_status(int cmd_no, int status);
    void set_time(unsigned int now_t);
    void amount_inc(int cmd_no, int result);
    void get_statiscs(int cmd_no, char*out_info, int len);
    
private:
    CmdStaticsNode* nodes_;
    int cmd_no_start_;      //起始命令字
    int cmd_no_end_;         //末尾的命令字
    unsigned int time_now_;              //当前的时间
    
};

#endif // _ISGW_OPER_BASE_H_
