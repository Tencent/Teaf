/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/

#ifndef _ISGW_OPER_BASE_H_
#define _ISGW_OPER_BASE_H_
#include "easyace_all.h"
#include "qmode_msg.h"
#include "cmd_amount_contrl.h"
#include "isgw_mgr_svc.h"

#ifndef FIELD_NAME_SVC
#define FIELD_NAME_SVC "service"
#endif

//��Ϣ����ָ���
enum _PDB_BASE_CMD
{
    // 100 �����ڲ����� 
    CMD_TEST = 1, //�ڲ�ѹ��������ָ�� 

    CMD_SELF_TEST = 2,					// ҵ���Բ�����
    CMD_GET_SERVER_VERSION = 3,		// ��ȡ��ǰsvr�İ汾��
    CMD_SYS_LOAD_CONF = 10,             //���¼���������Ϣ 
    CMD_SYS_GET_CONTRL_STAT = 11,       //��ѯ��������״̬
    CMD_SYS_SET_CONTRL_STAT = 12,       //��������״̬
    CMD_SYS_SET_CONTRL_SWITCH = 13,       //����ʱ�����������ƹ��ܿ���
    
    CMD_PDB_BASE_END
};

class IsgwOperBase
{
protected:
    static IsgwOperBase* oper_;
    
public:
    IsgwOperBase();
    virtual ~IsgwOperBase();
    
    static IsgwOperBase* instance(IsgwOperBase* oper);
    static IsgwOperBase* instance();
    static int  init_auth_cfg();//����Ȩ�޿����������ȡ

    virtual int init();
    virtual int process(QModeMsg& req, char* ack, int& ack_len);
    virtual int reload_config();
    virtual int time_out();
    virtual int handle_close(int fd);
    // wait task
    virtual int wait_task();

    int internal_process(QModeMsg& req, char* ack, int& ack_len);
    int is_auth(QModeMsg& req, char* ack, int& ack_len);

private:
	int self_test(int testlevel, std::string& msg);
	
private:
    // ���������Ʊ�־��Ϊ1���������ִ���
    static int cmd_auth_flag_;
    // ��֧���������б�
    static std::map<int, int> cmd_auth_map_;
    // ҵ�����Ʊ�־��Ϊ1����ҵ����
    static int svc_auth_flag_;
    // ��֧��ҵ���б�
    static std::map<int, int> svc_auth_map_;
    
};

#endif // _ISGW_OPER_BASE_H_
