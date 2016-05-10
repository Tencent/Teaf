/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
#ifndef ISGW_PROXY_H
#define ISGW_PROXY_H

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>

enum IsgwProxyErrorCode
{
    IPEC_NO_SERVER = -100,
    IPEC_SOCKET_ERROR,
    IPEC_CONNECT_FAIL,
    IPEC_SEND_FAIL,
    IPEC_RECV_FAIL,
    IPEC_TIMEOUT,
};

// 周期性从admin server取ip配置, admin server的地址通过dns解析获得
// 对每个server的访问采用Tcp短连接
// 容灾方式: tcp连接失败/访问超时"连续"超过一定次数, 则认为该server不可用, 下一周期再重试
// 负载均衡方式: 简单轮询, 但会跳过不可用server, 所有server不可用则随机选择一个
class IsgwProxy
{
    struct ServerStat
    {
        ServerStat() : total(0), fail(0) {}
        uint32_t total;
        uint32_t fail;
    };

    enum
    {
        DEF_TIMEOUT = 50,
        DEF_INTERVAL = 300,
        DEF_MAX_FAIL_TIMES = 10,
    };

public:
    // port 要访问server的端口号
    // timeout 连接server/接收返回的超时时间
    // refresh_server_interval 刷新server列表的时间间隔, 单位秒
    // max_fail_times, refresh_server_interval时间内如果连续访问失败超过该次数, 则在本周期内屏蔽对该server的访问
    IsgwProxy(uint16_t port, 
        uint32_t timeout = DEF_TIMEOUT, 
        uint32_t refresh_server_interval = DEF_INTERVAL, 
        uint32_t max_fail_times = DEF_MAX_FAIL_TIMES) 
            : last_refresh_time_(0), server_index_(0), port_(port), timeout_(timeout), 
            refresh_server_interval_(refresh_server_interval), max_fail_times_(max_fail_times)
    { 
        pthread_mutex_init(&mutex_, NULL); 
    }

    ~IsgwProxy() { pthread_mutex_destroy(&mutex_); }

public:
    // req 请求
    // rsp 回应
    int get(const std::string& req, std::string& rsp);

    // 从admin server取服务器ip列表
    // servers 输出服务IP列表
    // port 要请求服务的端口号
    // timeout 访问admin server的超时时间, 单位毫秒
    // 返回值 0成功 其他失败
    static int get_servers(uint16_t port, int32_t timeout, std::vector<std::string>& servers);

    // 指定ip, 端口, 超时信息访问接口
    static int get(const std::string& server, uint16_t port, int32_t timeout, const std::string& req, std::string& rsp);

    static std::map<std::string, std::string> parse(const std::string& rsp);

    static std::vector<std::string> split(const std::string& items, char delim);

private:
    // 取使用的服务器ip
    int get_server(std::string& server);

private:
    // server列表, 和每个server的失败次数
    std::vector<std::pair<std::string, ServerStat> > servers_;
    pthread_mutex_t mutex_;
    uint32_t last_refresh_time_;
    int32_t server_index_;

private:
    uint16_t port_;
    int32_t timeout_;
    uint32_t refresh_server_interval_;
    uint32_t max_fail_times_;

private:
    static const std::string ADMIN_SERVER1;
    static const std::string ADMIN_SERVER2;
    static const uint16_t ADMIN_PORT;
};

#endif //ISGW_PROXY_H

