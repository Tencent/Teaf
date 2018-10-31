## Teaf Introduction
Tencent Easy ACE Framework, ACE-based high-performance lightweight service framework, single-process multi-threading model, support multiple network IO models such as select/epoll, support tcp and udp protocols, support binary (pb, etc.) and text format ( Json et al., easy to understand), is easier to maintain and lighter than the framework of a multi-process model. The business side only needs to develop its own logical processing to achieve a high-performance business back-end server. It has been matured in most platform products of Tencent Entertainment (IEG), such as idip, game life, heart, help, new terminal game center aj, cross, etc. The company's other BG also has many products in use.

The specific detailed function list is as follows:
* Single-process multi-threaded model, simple operation and high performance;
* Support binary (pb, etc.) and text format (json, etc., easy to understand);
3. Command flow control, request volume monitoring and other characteristics;
4. Provide a variety of databases, storage access interface package, including mysql, redis, etc.;
5. Provide uniform access data collection (statistics);
6. Can support message routing and forwarding;
7. Provide batch processing features (usually used for bulk friend information query);
8. Support business control to return messages;
9. Supports both the synchronous and asynchronous connection management modes of the backend module;
10. Provide a lot of common tool functions or common classes, such as encryption and decryption, codec, character set conversion, etc.

Performance reference data: ordinary idc8 core server (tlinux2.0 intel 2.53G CPU 8G memory)
100+ client, running a single isgw/teaf server process, processing power is about 6w qps, cpu total occupancy is about 170% (divided by 8 is 21%, the cpu where the network interrupt is basically running)
Run 4 processes, processing capacity is around 23w qps.

#开发步骤
1. Inherit subclasses from IsgwOperBase
2. Reimplement IsgwOperBase* factory_method() Returns Inherited subclass
IsgwOperBase* factory_method()
{
    TempProxy::init();
    
    IsgwOperBase* obj = new PdbOper();
    Return obj;
}
3. Implement the process function in the subclass to implement the corresponding business logic
Int process(QModeMsg& req, char* ack, int& ack_len);
4. Compile and install

#样程序
There are several sample programs in the svr/ directory, such as pdb_oper.cpp *oper.cpp
You can compile the experience in the svr directory. Pay attention to the libraries you need to depend on when compiling (if you don't have mysql, you can delete the files related to db)

#问题和技术交QQ群 379103538

## Participation contribution

[Tencent Open Source Incentive Program] (https://opensource.tencent.com/contribution) Encourage developers to participate and contribute, and look forward to your joining.
