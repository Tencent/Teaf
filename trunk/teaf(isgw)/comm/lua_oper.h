/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
//
// Created by away on 15/6/16.
//

#ifndef ISGW_LUAOPER_H
#define ISGW_LUAOPER_H
#include "isgw_comm.h"

extern "C"{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
};

typedef map<unsigned int, lua_State *> LUA_STATE_MAP;

class LuaOper {
public:
    LuaOper(){};
    virtual ~LuaOper(){}
    static LuaOper* instance();

    lua_State * init();
    int process(const char *req, char* ack, int& ack_len);
    int close(lua_State *lua_state);

private:
    // 目前不支持多线程，需要优化成每个线程有自己的 lua_state
    static LUA_STATE_MAP state_map_;
    //static lua_State *lua_state_;
    static LuaOper* oper_;
};


#endif //ISGW_LUAOPER_H
