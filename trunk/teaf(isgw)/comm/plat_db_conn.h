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
* 连接类，单个连接的管理类，本身不保证线程安全，需要上层保证
* 主要实现以下功能:
* 1 数据库连接原始api的封装
* 2 连接的(状态)自我管理，自动重连，重连时设置需要的字符集等
******************************************************************************/

#ifndef _PLAT_DB_CONN_H_
#define _PLAT_DB_CONN_H_

#ifdef WIN32
#include <winsock2.h>
#endif

#include "mysql.h"
#include "errmsg.h"

#include <string>
#include <vector>
using namespace std;

#define PLAT_MAX_ERROR_LEN 255
#define PLAT_MAX_IP_LEN 64  //gcs的db域名可能会很长
#define PLAT_MAX_USER_LEN 24
#define PLAT_MAX_PSWD_LEN 24
#define PLAT_MAX_DBNM_LEN 24
#define PLAT_MAX_UXSK_LEN 128
#define PLAT_MAX_CHARSET_LEN 32 //字符集字符串大小

class PlatDbConn
{
public:
    PlatDbConn();
    ~PlatDbConn();
    // SELECT caller need to free result_set
    int exec_query(const char* sql, MYSQL_RES*& result_set);
    int exec_multi_query(const char* sql, vector<MYSQL_RES*> & result_set_list);
    // INSERT DELETE UPDATE
    int exec_update(const char* sql, int& last_insert_id, int& affected_rows);
    // TRANS 
    int exec_trans(const vector<string>& sqls, int& last_insert_id, int& affected_rows);
    int connect(const char* db_ip, const char* db_user, const char* db_pswd, 
        unsigned int port=0, int timeout=2, const char *db=NULL
        , const char *unix_socket=NULL, unsigned long client_flag=0); //CLIENT_MULTI_STATEMENTS
    int set_character_set(const char* character_set);
    unsigned long escape_string(char *to, const char *from, unsigned long length);

    char *get_err_msg(void) 
    {
        return err_msg_;
    }

private:
    int connect(); //内部自动重连用
    void disconnect(); //
    int set_character_set(); 

private:
    MYSQL mysql_;
    int conn_flag_; //连接是否有效的标识
    char db_ip_[PLAT_MAX_IP_LEN];
    char db_user_[PLAT_MAX_USER_LEN];
    char db_pswd_[PLAT_MAX_PSWD_LEN];
    
    char db_[PLAT_MAX_DBNM_LEN];
    unsigned int port_;
    char unix_socket_[PLAT_MAX_UXSK_LEN];
    unsigned long client_flag_;
    int time_out_; //超时时间，目前为连接有效

    char char_set_[PLAT_MAX_CHARSET_LEN];
    
    char err_msg_[PLAT_MAX_ERROR_LEN+1];
};

#endif //_PLAT_DB_CONN_H_

