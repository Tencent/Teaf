/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
#ifndef _RDS_ACCESS_H_
#define _RDS_ACCESS_H_

#include "isgw_comm.h"
#include <string>
#include "hiredis/hiredis.h"

struct SSPair{
    SSPair() : score(0), rank(0) {}
    string member;
    int64_t score;
    long long rank;
};

class RdsSvr {

public:
    struct Handle {
        Handle() : rc(NULL), mutex(NULL), ip(), port(0), dbid(0) {}
        redisContext *rc;
        ACE_Thread_Mutex *mutex;
        string ip;
        int port;
        struct timeval timeout;
        int dbid;
    };

public:
    //只配一个redis时才可调这个函数
    static RdsSvr &instance();

    //删除指定key
    int del_key(string dbid, string &uin, string &custom_key);
    //对指定key设置过期时间
    int expire_key(const string& dbid, const string& uin, const string& custom_key, int seconds, int flag=0);
    //得到redis服务器上的unix时间
    int redis_time(unsigned &tv_sec, unsigned &tv_usec);
    // 获取key过期时间
    int ttl(const std::string& key);
    //查询指定key下的string类型的数据
    int get_string_val(string dbid
                       , string &uin
                       , string &custom_key
                       , string &value);
    //设置指定key下的string类型的数据
    int set_string_val(string dbid
                        , string &uin
                        , string &custom_key
                        , string &value
                        , string expire="");
    //对指定key的string类型数据做增减操作
    //该key下的数据必须为数字
    int inc_string_val(string dbid
                        , string &uin
                        , string &custom_key
                        , string &value);

    // 更新指定key，并原子返回原先的值
    int get_set(const std::string& key
                        , const string& custom_key
                        , const std::string& nvalue
                        , std::string& ovalue);

    //查询指定key下sorted-set数据列表
    int get_sset_list(string dbid
                        , string &ssid
                        , vector<SSPair> &elems
                        , int start
                        , int num
                        , int flag);
    //查询指定key下sorted-set数据的指定member的排名
    int get_sset_rank(string dbid
                        , string &ssid
                        , vector<SSPair> &elems
                        , int flag);
    //更新指定key下sorted-set数据的指定member的score值
    //如果已经存在改member，则将其覆盖
    int set_sset_list(const string& dbid
                        , const string &ssid
                        , vector<SSPair> &elems
                        , int &num);
    //增量的设置指定每个member的score变化值
    int inc_sset_list(string dbid
                        , string &ssid
                        , SSPair &elem);
    //删除指定key下sorted-set数据的指定member
    int del_sset_list(string dbid
                        , string &ssid
                        , vector<SSPair> &elems
                        , int &num);
    //查询指定key下sorted-set数据的member个数
    int get_sset_num(string dbid, string &ssid);

    //获取hash表中指定field的value值
    int get_hash_field(string dbid
                        , const string &hkey
                        , const vector<string> &fields
                        , map<string, string> &value);
    //获取hash表中field/value列表
    int get_hash_field_all(string dbid, string &hkey, std::map<string, string> &elems);
    //设置hash表中指定field的value值
    int set_hash_field(string dbid, string &hkey, std::map<string, string> &elems);

    //增加hash表中指定field的值并返回
    int inc_hash_field(const string& dbid
                        , const string& hkey
                        , const string& field
                        , int& value);                        
    //删除hash表中指定field的value值
    int del_hash_field(string dbid, const string &hkey, const string &field);
    // 返回哈希表中field数目，失败返回-1
    int get_hash_field_num(const string &hkey);

    // 返回列表元素个数，默认dbid为0
    int get_list_num(const std::string& list);

    // 返回列表指定区间元素
    int get_list_range(const std::string& list
        , std::vector<std::string>& range_vec
        , int32_t start
        , int32_t stop);

    // 发送消息到指定频道
    int pub_msg(const std::string& channel, const std::string& msg);
    
    //执行指定的lua脚本
    int eval_exec(string dbid
                    , const string &script
                    , const string keys
                    , const string param
                    , int &res);
    // 执行指定的lua脚本(多参数版)
    int eval_multi_command(const string &script
        , const std::vector<std::string>& keys
        , const std::vector<std::string>& params
        , std::vector<std::string>& res
        , int32_t type = 0);
    
    RdsSvr();
    RdsSvr(string &redis);
    ~RdsSvr();
    int init(string redis_sec="redis");

private:
    template <typename T>
    std::string stringify(T input)
    {
        std::ostringstream oss;
        oss << input;
        return oss.str();
    }
    
    ACE_Thread_Mutex& get_handle(Handle * &h, int flag=0);
    void rst_handle(Handle * &h);
    int sel_database(Handle * &h, int dbid);
    int get_string_reply(Handle * &h
                        , redisReply * &reply
                        , string &key
                        , const string value=""
                        , const string expire=""
                        , const int flag=0);

    std::vector<Handle> m_redis_list_;
    std::vector<Handle> s_redis_list_;
    struct timeval timeout_;
    int port_;
    char master_ip_[128];
    char slave_ip_[128];
    char passwd_[64];
    int inited;
};

#endif 

