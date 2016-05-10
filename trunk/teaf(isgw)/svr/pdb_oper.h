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
 *  @file    pdb_oper.h
 *
 *  此文件为pdb内部保留的业务操作接口定义文件
 *  此文件可以作为框架的使用样例文件 
 *  
 *  @author xwfang
 */
//=============================================================================
#ifndef _PDB_OPER_
#define _PDB_OPER_
#include "isgw_comm.h"
#include "pdb_prot.h"
#include "isgw_oper_base.h"

#include "temp_proxy.h"

class PdbOper : public IsgwOperBase
{
public:
    PdbOper();
    ~PdbOper();

/** @name process
* 
*
* @param req 入参，请求消息
* @param ack 出参，存放返回的响应消息
* @param ack_len 入参兼出参，传入为ack指向的最大空间，
*                          传出为ack实际存放的消息长度
*
* @retval 0 成功
* @retval 其他值表示不 成功
*/
    int process(QModeMsg& req, char* ack, int& ack_len);
    
private:
    
private:
        
};

#endif //_PDB_OPER_
