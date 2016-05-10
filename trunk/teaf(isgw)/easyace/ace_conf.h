/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
#ifndef _ACE_CONF_H_
#define _ACE_CONF_H_
#include "ace_all.h"
#include <string>
#include <map>
#include <fstream>
using namespace std;

typedef map<string, string> SECTION_CONF_MAP;
typedef map<string, SECTION_CONF_MAP> CONFIG_MAP;

class ACEConf
{
public:
    string get_conf_file(){return conf_file_;};
    // 扩展支持多个文件加载配置 
    int load_conf(const string& conf_file="", int flag=0);
    int get_conf_int(const char* section, const char* name, int* value);
    int get_conf_uint(const char* section, const char* name, uint32_t* value);
    int get_conf_str(const char* section, const char* name,
        char* value, int buf_len);

    int write_conf_str(const char* section, const char* name, char* value);
    int write_conf_int(const char* section, const char* name, int value);
    int write_svr_conf(const std::string& svr_file = "");

    static ACEConf* instance();
    ~ACEConf();
    int release();
private:
    ACEConf();
    
    CONFIG_MAP conf_items_;
    ACE_Thread_Mutex conf_lock_;

    ACE_Configuration_Heap* ace_config_;

    string conf_file_;
    string old_conf_file_;

    static ACEConf* instance_;
};

typedef ACEConf SysConf; //兼容历史名称 

#endif // _ACE_CONF_H_

