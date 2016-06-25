#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Connector.h"
#include "ace/Log_Msg.h"
#include "ace/SOCK_Dgram.h"
#include "ace/OS.h"

#include "ace/Date_Time.h"

using namespace std;

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
    if(argc < 4 )
    {
        printf("Usage: %s <ip> <port> <content> [rtimes] [interval]\n", argv[0]);
        return 1;
    }
        
    char buffer[2048];
    
    int rtimes = 1; //执行次数
    if(argc > 4 )
    {
        rtimes = atoi(argv[4]);
    }
    
    int interval = 1;//执行间隔
    if(argc > 5 )
    {
        interval = atoi(argv[5]);
    }
    
    ACE_INET_Addr svr_addr(atoi(argv[2]),argv[1]);
    ACE_Time_Value time_out(1); //等待 1 s 
    ACE_Time_Value zero(0,0); //不等待 立即返回 
    ACE_Time_Value *time_null=NULL; //会导致永久阻塞
    
    int send_len = 0;
    int recv_len = 0;

#ifdef UDP

    ACE_INET_Addr my_addr; //ACE_static_cast (u_short, 10101)
    ACE_SOCK_Dgram udp(my_addr);//(my_addr)
    for(int i=0; i<rtimes; i++)
    {
        memset(buffer, 0, sizeof(buffer));
        send_len = snprintf(buffer, sizeof(buffer), "%s\r\n", argv[3]);
        
        ssize_t sent = udp.send (buffer, send_len, svr_addr);
        ACE_DEBUG((LM_INFO, 
            "[%D] send udp msg"		
            ",send=%d"
            ",msg is %s"     
            "\n"
            ,sent
            ,buffer
            ));
        
        memset(buffer, 0, sizeof(buffer));
        recv_len = udp.recv (buffer, sizeof(buffer), svr_addr, 0, &time_out);
        
        ACE_DEBUG((LM_INFO, 
            "[%D] recv udp msg"
            ",recv_len=%d"
            ",msg is %s"     
            "\n"
            ,recv_len
            ,buffer
            ));
        usleep(interval*1000);        
    }    
    udp.close ();
    
#else
       
    ACE_SOCK_Connector connector;
    ACE_SOCK_Stream peer;
    
    //peer.enable(ACE_NONBLOCK); //没效果，导致提示 Bad file descriptor 
    if(-1 == connector.connect(peer, svr_addr))
    {
        ACE_DEBUG((LM_INFO, 
                    "[%D] connect failed,ip=%s"
                    ",port=%s"                    
                    ",errno=%d"
                    ",errmsg=%s"
                    "\n"
                    , argv[1]
                    , argv[2]
                    ,errno
                    ,strerror(errno)
                    ));
        return -1;
    }
    
    for(int i=0; i<rtimes; i++)
    {
        memset(buffer, 0, sizeof(buffer));
#ifndef MSG_LEN_SIZE		
        send_len = snprintf(buffer, sizeof(buffer), "%s\n", argv[3]);
        send_len = peer.send(buffer, send_len);
        ACE_DEBUG((LM_INFO, 
            "[%D] send msg"                    
            ",errno=%d"
            ",errmsg=%s"
            ",send_len=%d"
            ",msg is %s" 
            ,errno
            ,strerror(errno)
            ,send_len
            ,buffer
            ));

        // 接收消息 
        memset(buffer, 0, sizeof(buffer));
        recv_len = peer.recv(buffer, sizeof(buffer), &time_out);
        if(recv_len>0)
        {
            ACE_DEBUG((LM_INFO, "[%D] recv_len=%d,msg is ", recv_len));
            write(1, buffer, recv_len);
            write(1, "\n", 2);
        }
        else if(0==recv_len)
        {
            printf("server closed socket\n");
        }
        else
        {
            if(ETIME==errno)
                printf("server timeout\n");
            else
                printf("server return error,errno=%d,errmsg=%s\n", errno, strerror(errno));
        }
#else
        snprintf(buffer+MSG_LEN_SIZE, sizeof(buffer)-MSG_LEN_SIZE, "%s\n", argv[3]);
                
        send_len = strlen(buffer+MSG_LEN_SIZE)+MSG_LEN_SIZE; 
        
        //int *p_len = (int*)buffer; //把前面字节设置为数据长度
        //*p_len = htonl(send_len-MSG_LEN_SIZE); //转成网络字节序
        uint16_t *p_len = (uint16_t*)buffer;
        *p_len = htons(send_len-MSG_LEN_SIZE);
        
        ACE_DEBUG((LM_INFO, 
            "[%D] send msg with head"
            ",send_len=%d"
            ",*p_len=%d"
            ",msg is %s"
            "\n"
            ,send_len
            ,ntohs(*p_len) //显示正常信息
            ,buffer+MSG_LEN_SIZE
            ));
        send_len = peer.send(buffer, send_len);
        ACE_DEBUG((LM_INFO,"[%D] send msg"
			",errno=%d,errmsg=%s"
			",ret=%d,msg=%s"
            , errno
            , strerror(errno)
            , send_len, buffer+MSG_LEN_SIZE
            ));
        
        // 接收消息
        memset(buffer, 0, sizeof(buffer));
        recv_len = peer.recv(buffer, sizeof(buffer), &time_out);
        if(recv_len>0)
        {
            ACE_DEBUG((LM_INFO, "[%D] recv_len=%d,msg is ", recv_len));
            write(1, buffer, recv_len);
            write(1, "\n", 2);
        }
        else if(0==recv_len)
        {
            printf("server closed socket\n");
        }
        else
        {
            if(ETIME==errno)
                printf("server timeout\n");
            else
                printf("server return error,errno=%d,errmsg=%s\n", errno, strerror(errno));
        }
#endif
        usleep(interval*1000);
    }

    //晚点退出，测试用 
    //sleep(100);
    peer.close();
#endif
    
    return 0;
}
