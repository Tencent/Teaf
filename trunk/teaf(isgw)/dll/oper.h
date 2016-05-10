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
 *  @file    oper.h
 *
 *  此文件为处理业务逻辑的头文件
 *  
 *  @author xwfang
 */
//=============================================================================

#ifndef _OPER_H_
#define _OPER_H_


extern "C" int oper_init();
extern "C" int oper_proc(char* req, char* ack, int& ack_len);

#endif // _OPER_H_
