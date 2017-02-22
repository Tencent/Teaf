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
* SOCKET连接池管理类(扩展)，对物理单元的个体(一个服务器或者更小的级别)进行连接管理
* 这连接池的任意两个连接理论上具有对等的逻辑功能，没有差别
* 为了保证长连接，请在调用接口中实现为静态对象或者全局对象
* 
* history:
* ....       init xwfang
* 2009-11-20 1 增加备机连接功能，每个连接管理器，支持配置备用 ip
*            2 优化代码直接把ip和端口信息读入类的成员变量
*            3 优化连接池管理，支持线程和连接不绑定，而是查找空闲的连接，解决suse版本的问题 
*            4 增加send_recv接口，避免同步模式中单独的recv使用导致连接使用错乱的问题
* 2009-12-29 1 增加连接管理策略的控制，支持指定失败次数和重连时间 
* 2010-03-02 1 修复 PlatConnMgr 可能有之前遗留的垃圾数据的问题
*            2 增加 PlatConnMgr 里面的 send_recv_ex 扩展接口 
* 2010-05-04 1 扩展为 PlatConnMgrEx 类 支持多个ip配置
*            2 并根据传入的 uin均匀分配 或者 随机路由(不传uin)
*            3 如果某个ip有问题，则使用他它的下一个ip的连接
* 2010-12-01 1 发送时支持二进制协议的数据 接收时只能是文本协议 
******************************************************************************/
#ifndef _PLAT_CONN_MGR_EX_H_
#define _PLAT_CONN_MGR_EX_H_
#include "easyace_all.h"

#define IP_NUM_MAX 20  //后端服务器 ip 最大个数 
#define IP_NUM_DEF 2  //后端服务器 ip 缺省个数 
#define POOL_CONN_DEF 20 //缺省每个ip xx个连接
#define POOL_CONN_MAX 100  //连接池单 ip 最大允许的连接数
#define MAX_ERROR_LEN 128 
#define MAX_SECTION_NAME_LEN 32 
#define SOCKET_TIME_OUT 150       //单位是ms
#define DEF_USE_STRATEGY 1
#define DEF_MAX_FAIL_TIMES 5
#define DEF_RECON_INTERVAL 10

//定义是否加锁处理
#ifndef GUARD_DOG_WATCH 
#define GUARD_DOG_WATCH(latch) ACE_Guard<ACE_Thread_Mutex> guard(latch)
#endif

class PlatConnMgrEx
{
public:
    PlatConnMgrEx();
    PlatConnMgrEx(const char*host_ip, int port, int conn_num, int time_out = 0);
    ~PlatConnMgrEx();
    ///通过配置初始化所有连接
    int init(const char *section = NULL, const std::vector<std::string> *ip_vec = NULL); 
    int send (const void *buf, int len, const unsigned int uin=0);
    int send (const void *buf, int len, const std::string& ip);
    //发送指定长度的数据，并且接收需要的数据，如果全部成功返回值为接收到的长度 
    int send_recv (const void *send_buf, int send_len, void *recv_buf
        , int recv_len, const unsigned int uin=0);
	//可以指定结束符的发送和接收数据
    int send_recv_ex (const void *send_buf, int send_len, void *recv_buf
        , int recv_len, const char* separator=NULL, const unsigned int uin=0);
    // 分步的 recv 使用只适用于异步模式，因为数据的发送和接收可能在不同连接上 
    int recv (void *buf, int len, const unsigned int uin=0);
    
private:
    //从连接池获取一个连接 注意 ip_idx 是有可能会被修改的 
    ACE_SOCK_Stream* get_conn(int index, int & ip_idx);
    
    //从连接池获取一个连接 直接返回 一个
    ACE_Thread_Mutex& get_conn(unsigned int uin, ACE_SOCK_Stream *& conn);
    ///通过指定ip和port 初始化第index个连接，不指定则用内部的 
    int init_conn(int index, int ip_idx, const char *ip = NULL, int port = 0); 
    int fini(int index, int ip_idx); //终止第index个连接 
    unsigned int get_index(int ip_idx, unsigned uin=0); //获得连接的下标 
    inline int get_ip_idx(unsigned int uin=0);
    inline int get_ip_idx(const std::string& ip);
    
private:
    // 连接及状态相关信息 
    ACE_SOCK_Stream* conn_[IP_NUM_MAX][POOL_CONN_MAX]; //连接指针 
    int conn_use_flag_[IP_NUM_MAX][POOL_CONN_MAX]; //连接的使用状态标志
    ACE_Thread_Mutex conn_lock_[IP_NUM_MAX][POOL_CONN_MAX]; //和连接一一对应保证单个线程使用连接
    //ACE_Thread_Mutex init_lock_[IP_NUM_MAX][POOL_CONN_MAX]; //保证初始化的正常 

    ACE_Thread_Mutex conn_mgr_lock_; // conn mgr 本身的锁，对所有连接操作时用此锁
    char section_[MAX_SECTION_NAME_LEN]; 
    int conn_nums_; //实际单个 ip 的连接总数
    ACE_Time_Value time_out_;
    int ip_num_; // 配置的服务器的 ip 总数 

    //服务器及状态相关信息 
    int fail_times_[IP_NUM_MAX]; //操作实际连续失败次数，包括发送，接收，等
    int last_fail_time_[IP_NUM_MAX]; //最后失败时间     
    char ip_[IP_NUM_MAX][INET6_ADDRSTRLEN]; 
    int port_; 
    
    //策略配置
    int use_strategy_; //策略使用标志 
    int max_fail_times_; //操作最多连续失败次数，包括发送，接收，等
    int recon_interval_; //重连间隔

};

#endif //_PLAT_CONN_MGR_EX_H_
