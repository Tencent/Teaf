编译安装指导(目前主要支持 linux 平台)：

1 编译：进入 svr 目录(样例程序，依赖于mysql，没安装可以删除vip_db_oper.*再编译)，修改makefile依赖的库路径

   如果makefile中定义了注释了 #ISGW_USE_DLL = 1， 则表示采用static编译，则需要安装以下包，以centos为例： yum install glibc-static  libstdc++-static zlib-static， 继续：
   
  ,执行  make conf; make      (make clean可以用来清理)

2 安装(可使用install.sh脚本进行自动安装)：建议目录结构为 bin cfg log 

3 启动 ：使用 start.sh 脚本进行即可 

4 测试 ：telnet 或者 用 client 目录下的 测试工具 client test.sh 脚本

5 性能参考：tlinux2.0  8核 intel 2.53G CPU  8G内存   100+客户端并发请求

  跑单个服务器进程，处理能力在6w qps左右，cpu总占用大概在170%左右（除以8就是21%，网络中断所在的cpu基本上跑满） 

  如果跑4个服务器进程，处理能力在23w qps

v3.4d100 2017-02

1 异步连接支持二进制协议 

2 修复小bug 


v3.3d501 2017-01

1 优化连接管理器 修复可能的bug等

2 优化转发模块的功能

v3.3d500 2016-10

1 redis管理类优化

2 工作队列支持多队列模型

3 性能优化，日志优化和bug修复等

4 优化异步联接管理器相关的功能

5 路由模块支持多连接等

v3.3d400 2016-04

1 redis 管理类优化

2 函数安全梳理

3 支持连接关闭回调业务函数 


v3.3d301 2015-08

1 增加对象数组池，支持高性能弱锁（或无锁）的对象管理

2 ISGWMgrSvc 消息处理方式优化



v3.3d300 2015-06

1 支持lua脚本，编译时指定 ISGW_USE_LUA 宏

2 优化qmode在大buff时可能core的情况 

3 增加数据库关键错误(1146,1054等)统计项



v3.3d203 2015-03

1 优化mysql多结果集释放

2 优化异步连接管理器状态判断(解决可能core的bug)

3 统计方式优化，不返回消息的请求也需要统计进来

  ，并且支持 0 命令字统计(因为修改了统计方式，需要和stat_tool同步更新，否则统计会错位)



v3.3d202 2014-11

1 优化对ACE6.2版本以上的支持，才用notify机制实现ack消息通知

2 优化目录结构，把老的 easyace 库分解成两部分，

  一部分依赖于ace的代码 放在 isgw/easyace/ 下，并入isgw库

  另外一部分跟 ace 无关的继续留下，改名为 comm 库，可以编译成 libcomm.a 使用 

3 增加连接管理器使用样例

4 优化反应器的连接支持 



v3.3d201 2014-11

1 支持断开空闲连接

2 对memset操作进行了大量优化（去除不必要的）

3 支持透传字段 _seqstr (同步模式)

4 优化 ack 连接管理 



v3.3d200 2014-07

1 修复CmdAmntCntrl的一个bug

2 QModeMsg 支持二进制协议，协议体需要应用层自己解析

3 异步连接管理器支持自定义消息id(用于唯一确定一条异步消息)解析函数

  ，将来可以支持idip等其他的后端平台或者服务



v3.3d100 2014-06

1 优化异步连接管理的使用方式，增加ASYTask类(独立线程)及相关代码。

2 优化异常日志和增加103指令用于测试



v3.2d211 2014-03

1 连接管理类支持毫秒级超时时间，默认超时时间150ms。

2 优化协议指定的转发功能，根据本地监听端口判断是否终止消息转发。

3 增加IBCOperFac::create_oper失败时异常处理。



v3.2d210 2013-12

1 优化路由支持功能，支持配置文件[router]段指定路由和内置的协议_appname参数指定路由两种方式。

2 内部协议 QModeMsg 类增加客户端连接端口号 sock_port_ 字段和 get_sock_port 函数。

3 连接管理类 PlatConnMgrEx 的 bug 优化。



v3.2d209 2013-09

1 链接管理类增加新的函数exec_multi_query支持select返回多个结果集

2 调整新建连接socket读写buffer大小为2*MAX_INNER_MSG_LEN

3 IBCMgrSvc批处理内部数据结构增加存储子任务结果的list字段；修改最终返回结果拼接方式，优先处理list字段；



v3.2d208 2013-08

1 连接管理类支持从db中加载server ip配置, 方便运维增加或下架机器. 

  server ip配置会保存到common节下svrs_file指定的文件中, 若找不到该配置, 默认保存到cfg/svrlist.ini文件中.

2 增加定时器的支持 开发者只需要重新实现 IsgwOperBase::time_out() 函数即可

	配置项为 

	# 信号处理和定时器的 handle

	[handle]

	# 是否使用定时器 0 不使用 默认值 1 使用 

	timer = 0 

	# 定时器间隔 单位 微秒 

	interval = 1000000

3 实现了内部指令 CMD_SYS_LOAD_CONF = 10 //重新加载配置信息 



v3.2d207 2013-05

1 修复数据库连接的bug

2 扩大了ip和host结构长度，支持域名访问

3 优化udp的协议处理，避免协议异常的问题

4 增加两个统计项，方便定位连接资源使用的问题

	STAT_CODE_DB_CONN_RUNOUT = 20259, //当前没有可用的DB连接

    STAT_CODE_TCP_CONN_RUNOUT = 20260, //当前没有可用的tcp连接

5 easyace/sys_comm.h 增加一些新的字符串解析函数 

6 优化部分日志 



v3.2d206 2012-10

1 优化udp协议支持，解决在大并发量情况下可能出现的句柄问题

2 优化代码，去掉部分snprintf前的多余memset操作



v3.2d205 2012-09

1 去掉ace5.4及更早版本的makefile支持 只支持5.6之后的ace版本

2 增加 comm/isgw_task_base.h 的 IsgwTaskBase 类 支持独立线程组 



v3.2d204 2012-09

1 ibc 模块支持int IBCOperBase::end(IBCRValue& rvalue) 回调函数

2 SysConf 支持加载多个配置文件 

3 统计模块初始化提前到工作线程池(ISGWMgrSvc)初始化之前

	防止出现在ISGWMgrSvc初始化中使用到连接管理类(PlatConnMgrEx), 然后该类默认初始化统计文件为".stat"的情况. 



v3.2d203 2012-05

1 支持tlinux下的编译

2 数据库连接管理支持存储过程

3 安全退出功能支持 



v3.2d202 2011-09

1 支持消息单纯的路由转发功能(可配置)

2 解决部分bug，去掉多余的工作线程锁等



v3.2d201 2011-02

1 支持21亿以上号码

2 优化连接管理

3 动态加载配置抽象接口

4 ACK 定时器时间可配置 



v3.2d200 2010-12

1 增加流量控制

2 和外部的接口优化为IsgwOperBase和IsgwOperBase* factory_method(){…}



v3.2d102 2010-10

1 增加工作线程放入响应模块队列超过(相对)的统计项

    STAT_CODE_PUT_ACK_TOOLATE = 20232, // 

2 支持 ISGWCIntf 的队列大小可配置 

    [isgw_cintf]

    # 单位字节

    quesize = 10485760

3 调整系统的一些默认配置

  超时时间 连接数等



v3.2d101 2010-09-06

1 支持 服务器启动时 自动获取本机 ip 进行监听 (同时支持udp协议)

    优先获取系统的配置，获取配置失败才获取本机 eth1 的ip

    [system]

    ip = 172.25.40.94

2 优化 *intf* 接口处理部分的日志内容(ip) 方便问题定位 



v3.2d100 2010-08-25

1 支持异步连接处理 

    增加了 PlatConnMgrAsy 异步连接管理类 使用方式保持跟之前类似 

    增加了 ISGWCIntf 处理后端的异步连接响应 

    增加了 异常统计项 STAT_CODE_ISGWC_ENQUEUE = 20261 //  isgwcintf 模块入自身消息队列失败

2 修复了一个 mysql db 连接默认多结果集的 bug 

3 ISGWAck 使用新的 send_n 发送方式 不使用 ACE_Message_Block 

4 增加了 ISGWAck 回送消息的各种异常统计

    STAT_CODE_ACK_DISCONN = 20252, // 回送消息时对端关闭

    STAT_CODE_ACK_BLOCK = 20253, // 回送消息时对端阻塞

    STAT_CODE_ACK_ABNOR = 20254, // 回送消息时异常

    STAT_CODE_ACK_UNCOMP = 20255, // 回送消息时不完全

5 修复了框架可能内存泄漏的问题(ACE_Reactor_Notification_Strategy 的问题)



v3.1d204 2010-08-02

1 去掉 PlatConnMgr 类 使用 PlatConnMgrEx 替代 并进行了优化 

2 调整 QModeMsg::get_result(...) 的实现，如果协议内容字段不存在则返回这个值  

    ERROR_NO_FIELD = -123456789



v3.1d203 2010-07-20

1 无符号 uin 支持

2 IBCMgrSvc 结果集的数量可配置

  [ibc_mgr_svc]  

  max_ibcr_record = 1000



v3.1d202 2010-05-20

1 统一通过 stat 模块进行统计，统计项的分配如下：

  1-10000 为对应的指令操作返回结果为0（即成功）的统计量

  10001-10240  可以留给业务自定义的统计项（正常异常都行）

  10241-20240 为对应的指令操作返回结果不为0（即失败）的统计量，规则为 cmd+10240（偏移量）

  20241 - 20480  为框架内部的异常统计项段

2 Qmode 类增加获取 cmd 和 uin 的接口(cmd和uin从消息体中解析获得)



v3.1d201 2010-05-12

1 增加 PlatConnMgrEx 类 支持以下特性 (总连接数 = ip数量 * 每个ip的连接数)

  a 支持多个ip配置

  b 并根据传入的 uin 均匀分配 或者 随机路由(不传uin)

  c 如果某个ip有问题，则使用他它的下一个ip的连接

  配置方式，在 svr 的配置段下面配置如下 :

  ip_num = 2 

  ip_0 = 172.16.197.19 

  ip_1 = 172.16.197.19 

2 前端请求过大，导致后台无法处理的时候主动断开连接，减少连接的资源占用时间，起到过载保护作用



v3.1d200 2010-04-30

1 ISGWMgrSvc 增加超时判断 可以通过以下配置选项进行设置

[isgw_mgr_svc]

# 默认超时不丢弃 

discard_flag = 0

# 默认的超时时间 10 S 

discard_time = 10



2 支持动态库加载，试用版本，编译时需要使用 -DISGW_USE_DLL 开关，并定义 ISGW_USE_DLL=1

并且配置文件进行如下配置

[isgw_mgr_svc]

# 指定动态库的名称 

dllname = oper



v3.1r100 2010-04-15

1 PlatConnMgr send.. 增加重发机制 避免存在连接被防火墙等断开的情况

2 QModeMsg 协议（构造函数）增加时间字段 通过 get_time() 函数获取



v3.0r100 正式版本 2010-03-24

1 增加了 ibc 模块的异常处理 日志信息等等

2 在好友和动态功能上验证试用过 

3 如果需要使用 ibc 模块请使用 -DISGW_USE_IBC 编译，并 把配置文件开关设置为 ibc_svc_flag=1 



v3.0d100 试用版本 2010-03-20

1 增加了ibc模块，可以支持批量处理功能，比较适合于好友关系链等需要批量操作的数据查询功能

2 优化了部分代码，比如 减少 strncpy 使用 



v2.0d210 2010-03-09

1 支持透传 _prot 可以支持多种协议的异步模式



v2.0d200 2010-03-03

1 支持透传 _sockfd _sock_seq _msg_seq 可以用来支持前端的异步模式调用  



v2.0d101 2010-03-02

1 调整ISGWUIntf,ISGWIntf,ISGWAck的相关日志，保持intf和ack模块日志格式和字段名称的一直性

2 修复 PlatConnMgr 可能有之前遗留的垃圾数据的问题

3 增加 PlatConnMgr 里面的 send_recv_ex 扩展接口 



v2.0d100 2010-01-29

1 增加回送消息的定时器触发机制，避免 notify 可能失效的情况

2 对系统的性能和稳定性有很大的优化



v1.6d107 2010-01-25

1 优化对象池回收机制，尽早进行对象回收，不等到网络数据发送完成，

  尽早减少内存占用，能减少 "dequeue failed" 出现的次数

  

v1.6d106 2010-01-23

1 对象池的返回bug处理：请求和响应对象池资源耗尽之后，动态分配内存的对象把内存占尽之后进行异常处理，避免进程异常退出

2 AceSockHdrBase 回送消息加上超时机制，避免消息对象被长时间占用不释放 

3 加入 Stat 类(stat.cpp stat.h) 方便系统按业务或者指令做流量统计，并输出到外部 映射的内存文件中 



v1.6d105 2009-12-29 

1 mysql数据库连接管理类和网络连接管理类增加连接策略机制，连接策略功能如下：

    每个连接管理的配置段支持配置以下参数 

    use_strategy = 1 # 是否使用连接策略，不配置默认不使用 

    max_fail_times = 3 # 最大允许的连续失败次数，超过则发到这个连接上的消息自动告知发送失败，不会尝试发送，直到超过重连间隔

    recon_interval = 20 # 重连间隔，单位 s 



v1.6d104 2009-12-07

1 对接收和发送消息模块的日志进行了细分，可以通过 log_mask 字段的 -p~NOTICE 或者 -pNOTICE 进行关闭或者开通，涉及到的日志有

    1 ISGWIntf putq msg to ISGWMgrSvc ...

    2 ISGWUIntf putq msg to ISGWMgrSvc ...

    3 put ISGWAck queue ok 

    4 AceSockHdrBase stat to send mb ... / AceSockHdrBase send mb ... succ ...



v1.6d103 2009-11-20

1 对 PlatConnMgr 类进行优化

    1 增加备机连接功能，每个连接管理器，支持配置备用 ip

    2 优化代码直接把ip和端口信息读入类的成员变量

    3 优化连接池管理，支持线程和连接不绑定，而是查找空闲的连接，解决suse版本的问题 

    4 增加send_recv接口，避免同步模式中单独的recv使用导致连接使用错乱的问题



v1.6d102 2009-11-06 

1 修改 PlatDbAccess 类，连接池模式支持两种模式：

    1 连接和线程绑定 通过 THREAD_BIND_CONN 控制 

    2 连接池在线程间共享 默认情况 

2 修改 PlatDbAccess 类的 exec_query 和 exec_update 接口 返回 mysql 的内部错误码 

  

v1.6d101 2009-08-25

1 加入版本说明文件，具体框架的介绍请参考 《商城svr框架设计和使用说明书.doc》



打包命令 :

tar -czvf isgw_v3.3d502.tar.gz isgw

