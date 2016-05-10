V1.8 201504
1 无任何新特性，大家不需要单独编译成库使用，直接链接文件使用即可

v1.7 201410
1 支持64位系统 tlinux ACE6.0等
2 去掉ACE5.4之前的版本支持
3 连接管理，消息队列优化等

v1.6.2
1 去掉 AceSockHdr 类 

v1.6.1 200908
1 增加了共享内存的bitmap类，可以在共享内存中申请一片内存作为bitmap使用，常用的场景为
  QQ号码 0 1 标志的判断，并且性能的要求比较高，必须在内存中进行

v1.6   200905
1 对sys_io_...类进行了优化，解决了可能存在的句柄泄漏的问题
2 对win下sys_io_...类的线程安全进行了优化

v1.5.3 200807
1 设置为TRACE日志: ACE_Object_Que 
2 增加 epoll 模式支持 ( 增加大并发接入支持 ) 

v1.5.2 200806 
1 为了调试方便，和业务的DEBUG日志区分开来，慢慢规范easyace库的日志开关为LM_TRACE，
  后续使用中屏蔽LM_TRACE日志请加上-p~TRACE(同时关闭DEBUG请用-p~DEBUG|~TRACE)
  目前完成的有ACESvc和SysConf类
  
2 优化了 ace_app 中 reactor 的初始化顺序
  
3 修改了 network_intf.h 文件的一个宏定义问题

4 开放 ace_svc 的消息队列大小设置接口
  
v1.5.1 200804 规范了部分模块的日志调试信息
  涉及 AceSockHdr sys_conf 等
  
v1.4 200803 实现网络通信通用的底层处理功能
1 AceSockHdr 类继承自 ACE_Svc_Handler 实现对格式为 {sizeof(int)+消息体} 的用户消息
  从网络层接收到用户数据空间并按照协议分段每个完整的消息，用户只需要重载
  virtual int process(...) 函数即可对消息包进行处理
  virtual int send(...) 函数用于完整的发送指定的消息块
  如果需要改变协议，采用文本协议，请重载 virtual int handle_input(...) 函数即可  
  涉及到的文件包括 ace_sock_hdr.h ace_sock_hdr.cpp 
  
2 加入网络编程(二进制协议中)编解码部分的通用函数(对基本数据类型的编解码)
  涉及的文件包括 sys_prot.h sys_prot.cpp
  
3 网络通信相关函数(同步模式，一般用于客户端api)，创建非阻塞模式的连接，
  (可以根据线程数自动产生同等数量的连接，以便防止网络阻塞，并保证线程安全)
  发送，接受指定的长度数据，对网络句柄进行管理等，函数列表存放在
  sys_cli_def.h(涉及的文件包括sys_thread_mutex.h sys_guard.h sys_cli_.*.cpp sys_cli_.*.h)
  
  支持配置多个服务器地址，配置文件路径有默认值，也可以通过 SYS_CONF_PATH 环境变量配置
  具体请参考 sys_cli_def.h 的定义     
  
  主要函数为：
  int SYS_connect(SYS_SOCKET& sock_fd, SYS_CONF_SERVER& server_conf
	, char* error);
  int SYS_send_n(SYS_SOCKET sock_fd, const char* buf, int len, char* error);
  int SYS_recv_n(SYS_SOCKET sock_fd, char* buf, int len, char* error);  
  
v1.3 200704 修改了 ACESvc 部分内容
  改进了 一些接口函数的参数类型，使得使用起来更方便，能在 process 等函数中增加应用的控制能力
  
v1.2 200608 更新说明
1 开放了 ACEApp 类的部分接口，比如进程的工作目录设置接口等
2 加入了xml ini文件解析相关的库 头文件 nds_config.h 具体使用说明请参考 nds_readme.txt

v1.1 200605 更新说明
1 修改了 ACEApp 类，调整了部分模块的顺序， 增加了Reactor初始化的接口，
  使得用户能使用自己的Reactor
2 增加了 ACESvc<typename IN_MSG_TYPE, typename OUT_MSG_TYPE> 模板，
  该类继承自ACE_Task_Base
  实现了生产消费者模式，消息处理器，消息只要从 putq(IN_MSG_TYPE* ) 放入，
  通过 process 处理之后，由send发送出去，每个流水线处理器是独立的可配置线程，
  使用者只需重载这些函数即可。
  
  虚函数说明：
   int ACESvc::putq(IN_MSG_TYPE* msg); 放入消息队列接口
   OUT_MSG_TYPE* ACESvc::process(IN_MSG_TYPE* msg); 消息处理函数
   int ACESvc::send(OUT_MSG_TYPE* ack = NULL); 消息处理完成之后调用的消息发送函数


v1.0 200503 使用说明
  easyace是基于ACE的应用程序库，目的是为了简化使用ACE实现server程序的工作。
  easyace中封装了读取 ini 配置、写日志、子进程管理、类CGI参数格式文本协议的解析
  字符串大小写转换等特性。使用easyace的应用程序开发者，可以通过override ACEApp类的
  几个虚函数实现应用逻辑。
  
  注：此库中只有ACE开头的类(或者文件)才是与ace相关的，其他的是与ace无关的可以通用。

功能列表:
1 通用类 sys_comm
  包含ace常用类
  普通字符串转16进制、16进展转普通字符串
  字符串转大写，小写
  解析cgi参数
2 配置文件处理类 sys_conf
  对 ini 段键格式的字符串进行解析处理
3 md5 tea加密解密算法
4 ace实现后台svr的模式
  简化svr后台模式的实现，如果需要改变一些行为只需要重载需要的函数即可。


使用方法：

1、编译安装

easyace依赖于ACE，因此开发人员应首先安装ACE库。ACE库安装好以后，可以进入
easyace目录，执行make完成easyace的编译。

2、应用程序开发

应用程序至少应该提供两个源文件，一个是主程序，一个是从ACEApp继承的应用程序类。

例如：
-------------------------------------------------------------------------------
my_app.h : 

#ifndef MY_APP_H_
#define MY_APP_H_

#include "easyace/ace_app.h"

class MyApp : public ACEApp
{
    // ...
};

#endif /* MY_APP_H_ */
-------------------------------------------------------------------------------
testace.cpp : 

#include "my_app.h"

int ACE_TMAIN (int argc, ACE_TCHAR* argv[])
{
    MyApp the_app;
    if (the_app.init(argc, argv) != 0)
    {
        return -1;
    }

    return 0;
}
-------------------------------------------------------------------------------

MyApp类应该override ACEApp中定义的几个虚函数。

以Makefile.app为模板，生成应用程序自己的Makefile，就可以进行编译了。

3、程序启动，停止

环境变量：
使用easyace开发的应用程序依赖于 <program name>_PATH环境变量，例如应用程序名为
foo的话，该环境变量名字为 FOO_PATH，该名字可以在命令行中修改。

目录结构：
$<program name>_PATH 是程序运行的当前目录，
<program name>.ini应该放置在该目录，<program name>.pid也将在该目录生成

$<program name>_PATH/log 放置应用程序日志文件

命令行参数：
-d 以daemon方式在后台运行，否则在前台运行
-p 指定应用程序名字，如果没有指定，则使用缺省值argv[0]
-c 指定子进程的数目

启动：
#export <program name>_PATH=.....
#$<program name>_PATH/<program name>

停止：
如果是前台运行，使用CTRL-C使之退出
如果是后台运行，cat $<program name>_PATH/<program name>.pid | xargs kill

4、虚函数说明
ACEApp::init_app()：进行程序启动的初始化工作，比如创建其它线程，或者侦听端口等。
ACEApp::quit_app()：进行程序退出的清理工作。
ACEApp::child_main()：对于多进程模型的程序，此函数是子进程的主函数。
ACEApp::quit_child()：对于多进程模型的程序，此函数是子进程的退出处理函数。
ACEApp::init_reactor：进行reactor的初始化，默认使用 ACE_Select_Reactor_N();

5、进程模型

使用easyace的应用程序可以选择单进程或多进程模型，对于多进程来说，父进程会负责
子进程的拉起和监控，如果子进程异常退出，父进程将把它重新拉起。

父进程和子进程缺省的主函数处理都是运行 ACE_Reactor 的主循环。

6、配置文件

使用easyace的应用程序在启动的时候就已经把配置文件读取到内存中，配置文件
是INI格式，在程序可以这样使用：
SysConf::instance()->get_conf_str
SysConf::instance()->get_conf_int
分别读取字符串和整型的配置项。

7、日志

[common]
log_mask = "-m 10240 -N 10 -f OSTREAM -p~DEBUG -s log/testace.log"

此配置项是用于日志的，可以指定循环日志的大小和个数，日志级别以及路径和文件名。
