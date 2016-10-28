/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
//=============================================================================
/**
 *  @file    pp_prot.h
 *
 *  此文件为svr框架对外的网络接口协议定义文件
 *
 *  @author xwfang
 */
//=============================================================================

#ifndef _PP_PORT_H_
#define _PP_PORT_H_
#include "isgw_config.h"
#include "sys/time.h"

#ifndef uint32 
typedef unsigned int uint32;
#endif 

#ifndef uint16 
typedef unsigned short uint16;
#endif 

//***********************************外部消息协议**********************************//
//网络底层的消息协议为MSG_SEPARATOR 宏定义分割的字符串信息作为一个完整的消息
//每个消息内部采用cgi的格式分割，即"cmd=001001&uin=88883960&...\n"
//对于业务商城的查询指令cmd用六位数字表示
//前三位由平台开发组分配给产品，后三位产品内部自己分配

//消息类型指令定义
typedef enum _PP_CMD
{
    CMD_PDB = 0, //1000 以内为 pdb 内部保留    
    CMD_FLASH_DLL = 100,

    CMD_DNF = 1000, //DNF业务开始

    CMD_FXD = 2000, //飞行岛业务开始

    CMD_WEBGAME = 3000, 

    CMD_DANCER = 4000, //QQ炫舞业务开始
    
    CMD_FC = 5000, //QQ飞车
    
    CMD_XX = 6000, //QQ寻仙

    CMD_FO = 7000, 
    
    CMD_FFO = 8000, 
    
    CMD_SG = 9000, 

    CMD_WEBDEV = 10000, // web 开发组使用开始

    CMD_QQTANG = 11000, // 

    CMD_CF = 12000, // 

    CMD_R2 = 13000, // 

    CMD_QQGAME = 14000, // 

    CMD_PM = 15000, // 
    
    CMD_AVA = 16000, // 

    CMD_DMLQ = 17000, // 

    CMD_TMXY = 18000, // 

    CMD_HXSJ = 19000, // 幻想世界 大碗菜

    CMD_OTOT = 20000, 

    CMD_XXZ = 21000,

    CMD_YKSK = 22000,  //烽火战国 

    CMD_PET_WORLD = 23000, //熊熊

    CMD_XY = 24000,     //轩辕

    CMD_LOL = 25000,        //英雄联盟

    CMD_YLZT = 31000,        // 御龙在天
    
    CMD_END //其他指令
}PP_CMD;

///接收缓冲区大小
#ifndef MAX_INNER_MSG_LEN
#define MAX_INNER_MSG_LEN 8400
#endif 

///内部预留的长度，为透传信息使用 
#define INNER_RES_MSG_LEN 192

///协议分割符
#ifndef MSG_SEPARATOR
#define MSG_SEPARATOR "\n"
#endif 

///命令字字段名
#ifndef FIELD_NAME_CMD 
#define FIELD_NAME_CMD "cmd"
#endif 

///模块版本号
#ifndef SERVER_VERSION 
#define SERVER_VERSION "isgw_v3.4"
#endif 

///其他字段名(可选)
#define FIELD_NAME_RESULT "result"
#define FIELD_NAME_UIN "uin"
#define FIELD_NAME_USER "user"
#define FIELD_NAME_PASSWD "passwd"
#define FIELD_NAME_INFO "info" //响应消息的说明信息

//操作权限定义
#define OP_GROUP_ROOT 1 
#define OP_GROUP_ADMIN 2 
#define OP_GROUP_USER 3 

//协议类型:
typedef enum _PROTOCOL_TYPE
{
    PROT_TCP_IP=0, //TCP IP 协议
    PROT_UDP_IP, //UDP IP 协议
}PROTOCOL_TYPE;

//***********************************外层消息协议**********************************//
/*
//请求消息
typedef struct stPriProReq
{
    uint32 sock_fd; //socket 的文件描述符
    uint32 protocol; //协议类型
    uint32 sock_ip; //
    uint32 sock_port; //
    uint32 sock_seq; //socket的序列号，sock_fd值相同时可以用来区分
    uint32 seq_no; //消息的序列号，唯一标识一个请求
    uint32 cmd;     //命令字
    struct timeval tv_time;   //消息获得时间
    uint32 rflag; // 消息是否透传的标志
    uint32 index; //消息在队列中的下标
    uint32 msg_len; //后面的有效消息长度
    char msg[MAX_INNER_MSG_LEN+1]; //接受到的消息体
    stPriProReq():sock_fd(0),protocol(0),sock_ip(0),sock_port(0),sock_seq(0)
        ,seq_no(0),cmd(0),rflag(0),msg_len(0){}; //,msg({0}) 
}PriProReq;
*/

//响应消息，头部和请求协议的同名字段同义，处理的时候需要把请求消息的头部保存进来
typedef struct stPriProAck
{
    uint32 sock_fd;
    uint32 protocol; //协议类型
    uint32 sock_ip; //
    uint32 sock_port; //
    uint32 sock_seq;
    uint32 seq_no;
    uint32 cmd;     //命令字
    struct timeval tv_time;   //对应请求消息获得时间
    struct timeval p_start;   //对应请求开始被工作线程处理的时间
    uint32 rflag; // 消息是否透传的标志
    uint32 index; //消息在队列中的下标
    uint32 time; //原始请求收到的时间
    uint32 total_span;      //本请求总的处理时间(包括procs_span),单位为100us
    uint32 procs_span;    //本请求在响应处理函数中的处理时间,单位为100us
    int ret;      //本请求的返回状态
    uint32 msg_len; //后面的有效消息长度
    char msg[MAX_INNER_MSG_LEN+1]; //存放结果，包括返回值和相应的信息，如果错误时存放错误描述
    stPriProAck():sock_fd(0),protocol(0),sock_ip(0),sock_port(0),sock_seq(0)
        ,seq_no(0),cmd(0),rflag(0),index(0),time(0),total_span(0),procs_span(0),ret(0)
        ,msg_len(0){}; //,msg({0}) 
}PriProAck;

typedef PriProAck PPMsg;

/******************************************************************************
// 常用错误定义
// 0 表示正常/成功
// 负数(<0) 表示系统级别异常 
//          比如配置问题 网络问题 需要人工干预修复的
//          这种异常通常是不应该存在的 需要高度关注的 
//          不然会导致系统严重不可用 或者不处理会有很大的潜在风险 
// 正数(>0) 表示业务逻辑级别异常 
//          比如记录不存在 记录重复等 
//          这种情况是可以存在的 但是应该尽量减少
******************************************************************************/
typedef enum _PP_ERROR
{
    ERROR_NO_FIELD = -123456789, //协议字段不存在 业务定义的返回尽量避免和这个定义冲突

    //从 (-10001)-(-11000 )为前端访问外部接口的错误 
    ERROR_OIDB = -10002, //OIDB 错误
    ERROR_MP = -10001, //营销平台错误 
    
    ERROR_FORWARD = -11, //请求转发失败
    ERROR_TIMEOUT_SER = -10, //业务接口发现消息超时 
    ERROR_IBC_FAC = -8, //ibc fac 异常
    ERROR_DECODE = -7, //内部协议(内容)解析非法 
    ERROR_REJECT = -6, //拒绝提供服务
    ERROR_NO_PERMIT = -5, //无权限访问(系统级别)
    ERROR_PARA_ILL = -4, //参数非法
    ERROR_TIMEOUT_FRM = -3, //框架内部发现消息超时 
    ERROR_CONFIG = -2, //配置异常 
    ERROR_NET = -1, //网络异常
    ERROR_OK = 0, //正常情况 
    ERROR_NOT_FOUND = 1, //业务接口返回记录不存在 或者数据库中无记录
    ERROR_DUPLICATE = 2, //业务接口返回记录重复 或者数据库中记录重复
    ERROR_MYSQL_NOAFFTD = 3, // mysql 操作受影响的行数为 0 
    ERROR_SERV_NO_PERMIT = 4, // 用户没权限操作(业务逻辑级别)
    
}PP_ERROR;

#endif // _PP_PORT_H_
