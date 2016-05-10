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
*  @file      ibc_oper_fac.h
*  @author xwfang
*  @history 
*  isgw 框架的 批量处理模块 
*  使用者请扩展和实现此类 
******************************************************************************/
#ifndef _IBC_OPER_FAC_H_
#define _IBC_OPER_FAC_H_
#include "ibc_oper_base.h"

class IBCOperFac
{
public:
    static IBCOperBase* create_oper(int cmd);
private:
};

#endif //_IBC_OPER_FAC_H_
