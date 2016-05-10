/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
#include <iostream>
#include <string>

using namespace std;

class person
{
public:
    person(const string& name, unsigned int age);

    ~person();

    string get_person_name() const;

private:
    string m_name;
    unsigned int m_age;
};


//向外暴露这个接口，使用" extern "C" "来声明之，否则待会儿调用symbol来获取
//本函数的地址时就必须填写mingle之后的全名
extern "C" person create_person(const string& name, unsigned int age);
