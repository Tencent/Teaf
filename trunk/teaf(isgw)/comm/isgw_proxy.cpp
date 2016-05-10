#include "isgw_proxy.h"
#include <iostream>

const std::string IsgwProxy::ADMIN_SERVER1("172.27.128.79");
const std::string IsgwProxy::ADMIN_SERVER2("10.130.136.205");
const uint16_t IsgwProxy::ADMIN_PORT = 6888;

int IsgwProxy::get(const std::string& req, std::string& rsp)
{
    // 取服务器ip
    std::string server;
    int ret = get_server(server);
    if(ret != 0)
    {
        return ret;
    }

    // 发送请求, 接收回应
    ret = get(server, port_, timeout_, req, rsp);

    // 统计
    pthread_mutex_lock(&mutex_);
    for(std::vector<std::pair<std::string, ServerStat> >::iterator it = servers_.begin();
        it != servers_.end(); ++it)
    {
        if(server != it->first)
        {
            continue;
        }

        ++it->second.total;
        if(ret != 0)
        {
            ++it->second.fail;
        }
        else
        {
            it->second.fail = 0;
        }
    }

    pthread_mutex_unlock(&mutex_);
    return ret;
}

std::map<std::string, std::string> IsgwProxy::parse(const std::string& input)
{
    std::map<std::string, std::string> data;
    std::string name, value;
    std::string::size_type pos;
    std::string::size_type oldPos = 0;

    // Parse the input in one fell swoop for efficiency
    while(true) 
    {
        // Find the '=' separating the name from its value
        pos = input.find_first_of("=", oldPos);

        // If no '=', we're finished
        if(std::string::npos == pos)
        {
            break;
        }

        // Decode the name
        name = input.substr(oldPos, pos - oldPos);
        oldPos = ++pos;
        
        // Find the '&' separating subsequent name/value pairs
        pos = input.find_first_of("&", oldPos);

        // Even if an '&' wasn't found the rest of the string is a value
        value = input.substr(oldPos, pos - oldPos);

        // Store the pair
        data[name] = value;
        if(std::string::npos == pos)
        {
            break;
        }

        // Update parse position
        oldPos = ++pos;
    }
    
    return data;
}

std::vector<std::string> IsgwProxy::split(const std::string& src, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(src);
    std::string item;
    while (std::getline(ss, item, delim)) 
    {
        if(!item.empty())
        {
            elems.push_back(item);
        }
    }
    return elems;
}

int IsgwProxy::get_server(std::string& server)
{
    unsigned now = static_cast<unsigned>(time(0));
    if(now > last_refresh_time_ + refresh_server_interval_)
    {
        std::vector<std::string> servers;
        int ret = get_servers(port_, timeout_, servers);
        if(ret == 0 && now > last_refresh_time_ + refresh_server_interval_)
        {
            pthread_mutex_lock(&mutex_);

            // 打印各server的访问次数
            for(std::vector<std::pair<std::string, ServerStat> >::const_iterator cit = servers_.begin();
                cit != servers_.end(); ++cit)
            {
                std::cout << cit->first << ": " << cit->second.total << " " << cit->second.fail << std::endl;
            }
            
            servers_.clear();
            for(std::vector<std::string>::const_iterator cit = servers.begin(); 
                cit != servers.end(); ++cit)
            {
                std::pair<std::string, ServerStat> elem;
                elem.first = *cit;
                servers_.push_back(elem);
            }

            last_refresh_time_ = now;
            server_index_ = 0;

            pthread_mutex_unlock(&mutex_);
        }
    }

    pthread_mutex_lock(&mutex_);
    size_t size = servers_.size();
    if(size == 0)
    {
        pthread_mutex_unlock(&mutex_);
        return IPEC_NO_SERVER;
    }

    // 取下一个可用的server
    int index = (server_index_ + 1) % size;
    for(; index != server_index_; index = (index + 1) % size)
    {
        if(servers_[index].second.fail < max_fail_times_)
        {
            break;
        }
    }

    // 所有server均不可用, 就取下一个
    if(index == server_index_)
    {
        index = (server_index_ + 1) % size;
    }

    server = servers_[index].first;
    server_index_ = index;
    pthread_mutex_unlock(&mutex_);
    return 0;
}

int IsgwProxy::get_servers(uint16_t port, int32_t timeout, std::vector<std::string>& servers)
{
    std::vector<std::string> admin_servers;

    // get admin server from dns
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int ret = ::getaddrinfo("admin.igame.ied.com", "", &hints, &res);
    if(ret == 0)
    {
        for(struct addrinfo* p = res; p != NULL; p = p->ai_next)
        {
            char ip[INET6_ADDRSTRLEN] = {0};
            ::inet_ntop(p->ai_family, &((struct sockaddr_in *)p->ai_addr)->sin_addr, ip, sizeof(ip));
            if(ip[0] == '\0')
            {
                continue;
            }

            std::cout << "get admin server from dns: " << ip << std::endl;
            admin_servers.push_back(ip);
        }

        freeaddrinfo(res); 
    }

    // DNS解析失败时, 用写死的ip
    if(admin_servers.empty())
    {
        admin_servers.push_back(ADMIN_SERVER1);
        admin_servers.push_back(ADMIN_SERVER2);
    }

    // 从admin server请求服务IP列表
    ret = 0;
    for(std::vector<std::string>::const_iterator cit = admin_servers.begin();
        cit != admin_servers.end(); ++cit)
    {
        std::ostringstream oss;
        oss << "cmd=921&port=" << port << "&expire=" << DEF_INTERVAL << "\n";

        std::string rsp;
        ret = get(*cit, ADMIN_PORT, timeout, oss.str(), rsp);
        if(ret != 0)
        {   
            continue;
        }

        // 解析rsp
        std::map<std::string, std::string> data = parse(rsp);
        ret = data.count("result") ? atoi(data.at("result").c_str()) : -1;
        if(ret != 0)
        {
            continue;
        }

        servers = split(data["info"], ' ');
        if(!servers.empty())
        {
            break;
        }
    }

    if(ret != 0)
    {
        return ret;
    }

    if(servers.empty())
    {
        return IPEC_NO_SERVER;
    }

    return 0;
}

int IsgwProxy::get(const std::string& server, uint16_t port, int32_t timeout, const std::string& req, std::string& rsp)
{
    // 建立套接字
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        return IPEC_SOCKET_ERROR;
    }

    int flags = -1;
    if(flags = ::fcntl(fd, F_GETFL), flags == -1)
    {
        ::close(fd);
        return IPEC_SOCKET_ERROR;
    }

    flags |= O_NONBLOCK;
    if(::fcntl(fd, F_SETFL, flags) == -1)
    {
        ::close(fd);
        return IPEC_SOCKET_ERROR;
    }

    // 连接
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ::inet_addr(server.c_str());
    int ret = ::connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1 && errno != EINPROGRESS)
    {
        ::close(fd);
        return IPEC_CONNECT_FAIL;
    }

    // 非阻塞的connect
    if(ret != 0)
    {
        struct pollfd fds[1];
        fds[0].fd = fd;
        fds[0].events = POLLOUT;

        ret = ::poll(fds, 1, timeout);
        if(ret == -1)
        {
            ::close(fd);
            return IPEC_CONNECT_FAIL;
        }

        if(ret == 0)
        {
            ::close(fd);
            return IPEC_TIMEOUT;
        }

        int err = 0;
        socklen_t errlen = static_cast<socklen_t>(sizeof(err));
        ret = ::getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&err, &errlen);
        if(ret == -1)
        {
            ::close(fd);
            return IPEC_CONNECT_FAIL;
        }

        if(err)
        {
            ::close(fd);
            return IPEC_CONNECT_FAIL;
        }
    }

    // 发送
    ret = ::send(fd, req.c_str(), req.size(), 0);
    // 发送失败或发送不完全
    if(ret == -1 || ret != static_cast<int>(req.size()))
    {
        ::close(fd);
        return IPEC_SEND_FAIL;
    }
    
    // 接收
    rsp.clear();
    struct timeval start, end;
    for(::gettimeofday(&start, NULL); 
        timeout >= 0; 
        ::gettimeofday(&end, NULL), timeout -= ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec)) / 1000)
    {
        struct pollfd fds[1];
        fds[0].fd = fd;
        fds[0].events = POLLIN | POLLPRI;

        ret = ::poll(fds, 1, timeout);
        if(ret == -1)
        {
            ::close(fd);
            return IPEC_RECV_FAIL;
        }

        if(ret == 0)
        {
            ::close(fd);
            return IPEC_TIMEOUT;
        }

        char buf[65536] = {0};
        ret = ::recv(fd, buf, sizeof(buf), 0);
        if(ret == -1)
        {
            ::close(fd);
            return IPEC_RECV_FAIL;
        }

        size_t i = 0;
        for(; i < sizeof(buf) && buf[i] != '\n'; ++i);
        rsp.append(buf, i);
        if(buf[i] == '\n')
        {
            ::close(fd);
            return 0;
        }
    }

    ::close(fd);
    return IPEC_TIMEOUT;
}

#if 0
int main()
{
    IsgwProxy proxy(5696, 50, 60);
    std::string rsp;
    proxy.get("cmd=434&uin=476794193\n", rsp);
    std::cout << rsp << std::endl;

    for(int i = 0; i < 100000; ++i)
    {
        proxy.get("cmd=434&uin=476794193\n", rsp);
        // std::cout << rsp << std::endl;
        ::usleep(10000);
    }
}

#endif





