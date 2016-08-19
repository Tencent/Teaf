# Teaf 简介
Tencent Easy ACE Framework (简称Teaf)，基于ACE的高性能轻量级服务框架，单进程多线程模型，支持select/epoll等多种网络IO模型，同时支持tcp和udp协议，支持二进制(pb等)和文本格式，相对多进程模型的框架来说更易维护，更轻量。业务侧只需要开发自己的逻辑处理即可实现高性能的业务后台服务器。已经在互娱(IEG)大部分平台类产品中成熟应用，比如idip，游戏人生，心悦，帮帮，新终端游戏中心aj，cross等，公司其他BG也有很多产品在使用。

具体的详细功能列表如下：
* 单进程多线程模型，运营简单，相比于多进程模型有更高的性能和更少的cpu资源消耗；
* 使用文本的协议(自定义或者json)，易于理解，开发成本低；新增二进制（比如pb等）协议格式的支持；
3. 指令流量控制、请求量监控等特性；
4. 提供多种数据库，存储访问接口封装，包括mysql， redis等；
5. 提供统一的访问量数据采集（统计）；
6. 可以支持消息路由转发；
7. 提供批量处理特性(常用于批量的好友信息查询）；
8. 支持业务控制是否返回消息；
9. 支持和后端模块同步和异步两种连接管理模式；
10. 提供很多公共的工具函数或者常用类，比如加解密，编解码，字符集转换等；

性能参考数据：普通idc8核服务器(tlinux2.0   intel 2.53G CPU    8G 内存)
100+客户端，跑单个isgw/teaf 服务器进程，处理能力大概在6w qps，cpu总占用大概在170%（除以8就是21%，网络中断所在的cpu基本上跑满）
跑4个进程，处理能力在23w qps左右。

#开发步骤
1. 从 IsgwOperBase 继承子类
2. 重新实现 IsgwOperBase* factory_method() 返回 继承的子类
IsgwOperBase* factory_method()
{
    TempProxy::init();
    
    IsgwOperBase* obj = new PdbOper();
    return obj;
}
3. 实现子类中的 process函数 实现相应的业务逻辑
int process(QModeMsg& req, char* ack, int& ack_len);
4. 编译及安装

#样例程序
svr/ 目录下有几个样例程序 比如 pdb_oper.cpp  *oper.cpp
可以在svr目录下 make 编译体验一下 编译的时候注意需要依赖的库(如果没有mysql可以删掉跟db相关的文件即可)
