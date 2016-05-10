/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
/******************************************************************************
 *  @file    ibc_oper_base.h
 *
 *  此文件为内部批量处理业务逻辑的基础类
 *  使用者请扩展和实现此类 
 ******************************************************************************/
#ifndef _IBC_OPER_BASE_H_
#define _IBC_OPER_BASE_H_
#include "easyace_all.h"
#include "qmode_msg.h"
#include "ibc_prot.h"

class IBCOperBase
{
public:
    IBCOperBase() {};
    virtual ~IBCOperBase() {}; 
    virtual int process(QModeMsg& req, char* ack, int& ack_len); 
    //这个为结果合并的逻辑，建议要做的非常轻量级 避免对其他线程有影响 
    virtual int merge(IBCRValue& rvalue, const char* new_item); 

	//合并后的最终回调，注意这个只会在全部结果处理完的时候最后调用一次
	virtual int end(IBCRValue& rvalue); 

private:

};

#endif // _IBC_OPER_BASE_H_
