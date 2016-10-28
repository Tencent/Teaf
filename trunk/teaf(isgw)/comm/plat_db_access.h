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
* MYSQL连接池管理类，对物理单元的个体(一个服务器或者更小的级别)进行连接管理
* 这连接池的任意两个连接理论上具有对等的逻辑功能，没有差别
* 为了保证长连接，请在调用接口中实现为静态对象或者全局对象
* 
******************************************************************************/
#ifndef _PLAT_DB_ACCESS_H_
#define _PLAT_DB_ACCESS_H_

#include "easyace_all.h"
#include "plat_db_conn.h"
#include "isgw_comm.h"

#define MAX_SECTION_NAME_LEN 32
#define MAX_DB_NAME_LEN 128
#define MAX_DB_HOST_LEN 64
#define MAX_DB_USER_LEN 32
#define MAX_DB_PSWD_LEN 64
#define MAX_ERROR_LEN 128
#define DB_CONN_MAX 200  //DB 连接池最大连接数
#define POOL_CONN_DEF 20  //连接池缺省连接数

//typedef unsigned int UINT;

class PlatDbAccess
{
public:
    PlatDbAccess();
    ~PlatDbAccess();
    int init(const char *section = "database"); //初始化所有连接
    int init(const char *host, const char *user, const char *passwd, int port
        , int index); //指定参数初始化连接
    int set_conns_char_set(const char* character_set); //把该连接池的所有连接设置为需要的字符集
    unsigned long escape_string(char *to, const char *from, unsigned long length);
    unsigned long escape_string(string & to, const string & from);
    // SELECT caller need to free result_set
    int exec_query(const char* sql, MYSQL_RES*& result_set, unsigned int uin=0);
    int exec_multi_query(const char* sql, vector<MYSQL_RES*>& result_set_list, unsigned int uin=0);
    // INSERT DELETE UPDATE
    int exec_update(const char* sql, int& last_insert_id, int& affected_rows
        , unsigned int uin=0);

    // TRANS
    int exec_trans(const vector<string>& sqls, int& last_insert_id, int& affected_rows
        , unsigned int uin=0);

    int free_result(MYSQL_RES*& game_res);
    int free_result(vector<MYSQL_RES*>& result_set_vec);
    
private:
    int fini(int index); //终止第index个连接
    PlatDbConn* get_db_conn(int index); //从连接池获取一个连接，第index个，保证和锁的序号一致性
    unsigned int get_conn_index(unsigned int uin=0); //获得连接的下标
    int32_t stat_return(const int32_t result, timeval* start, const char* sql="");
    int is_legal(const char* sql);

private:
    PlatDbConn* db_conn_[DB_CONN_MAX];
    int db_conn_flag_[DB_CONN_MAX]; //DB连接的使用状态 
    ACE_Thread_Mutex db_conn_lock_[DB_CONN_MAX]; //和连接一一对应保证单个线程使用连接
    int conn_nums_; //连接总数 
    char section_[MAX_SECTION_NAME_LEN]; 

    //服务器配置
    char db_host_[MAX_DB_HOST_LEN];
    char db_user_[MAX_DB_USER_LEN];
    char db_pswd_[MAX_DB_PSWD_LEN];
    int port_; 
    int time_out_; //db请求的超时时间

    //策略配置
    int use_strategy_; //策略使用标志 
    int max_fail_times_; //操作最多连续失败次数，包括发送，接收，等
    int fail_times_; //操作实际连续失败次数，包括发送，接收，等
    int recon_interval_; //重连间隔
    int last_fail_time_; //最后失败时间 
    
    char err_msg_[MAX_ERROR_LEN+1];
};

#endif //_PLAT_DB_ACCESS_H_
