/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
/*******************************************************************************
 *  @file    isgw_config.h
 *
 *  此文件为对svr框架定义的配置文件，可以在此文件中定义一下宏的值，
 *  改变系统的默认设置。
 *
 *  @author xwfang
 ******************************************************************************/

#ifndef _ISGW_CONFIG_H_
#define _ISGW_CONFIG_H_

#define MAX_INNER_MSG_LEN 4096
//#define MSG_SEPARATOR "\r\n"

// 指定消息头长度
//#define MSG_LEN_SIZE 4  // ISGWIntf 使用 ，目前只支持 2 和4 两个值
//#define MSG_LEN_SIZE_C 4  // ISGWCIntf 使用 ，此值目前应该只支持 4 一个取值 
//#define FIELD_NAME_CMD "cmd"

//超时告警的时间
//#define ALARM_TIMEOUT 1
#define OBJECT_QUEUE_SIZE 5000
//用于支持空闲连接清理，配置了就表示要清理
//#define MAX_IDLE_TIME_SEC 600 //单位 s 

#endif //_ISGW_CONFIG_H_
