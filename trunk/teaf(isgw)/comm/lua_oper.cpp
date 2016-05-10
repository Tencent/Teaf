//
// Created by away on 15/6/16.
//

#include <cstddef>
#include "lua_oper.h"

LuaOper* LuaOper::oper_;
//lua_State* LuaOper::lua_state_;
LUA_STATE_MAP LuaOper::state_map_;

LuaOper* LuaOper::instance()
{
    // 暂时不考虑加锁 只在单线程中执行
    if (NULL == oper_)
    {
        oper_ = new LuaOper;
    }
    return oper_;
}

lua_State * LuaOper::init()
{
    unsigned int threadid = syscall(__NR_gettid); //ACE_OS::thr_self();
    lua_State *lua_state = state_map_[threadid];

    if(lua_state != NULL)
    {
        close(lua_state);
    }

    lua_state = luaL_newstate();
    luaopen_base(lua_state);
    luaL_openlibs(lua_state);
    luaL_loadfile(lua_state, "./process.lua");

    //好像必须调用 不然函数找不到
    lua_pcall(lua_state, 0, LUA_MULTRET, 0);

    state_map_[threadid] = lua_state;
    ACE_DEBUG((LM_INFO, "[%D] LuaOper init lua succ threadid=%d\n", threadid));

    return state_map_[threadid];
}

int LuaOper::process(const char *req, char* ack, int& ack_len)
{
    unsigned int threadid = syscall(__NR_gettid); //ACE_OS::thr_self();
    lua_State *lua_state = state_map_[threadid];
    if(lua_state == NULL)
    {
        lua_state = init();
    }

    size_t len;
    lua_getglobal(lua_state, "process");
    lua_pushstring(lua_state, req);
    lua_call(lua_state, 1, 1);
    snprintf(ack, ack_len, "%s", lua_tolstring(lua_state, -1, &len));
    ack_len = len;
    lua_pop(lua_state, 1);
    return 0;
}

int LuaOper::close(lua_State *lua_state)
{
    if(lua_state != NULL)
    {
        lua_close(lua_state);
        lua_state = NULL;
    }
    else
    {
        LUA_STATE_MAP::iterator it;
        for(it=state_map_.begin(); it!=state_map_.end(); it++)
        {
            lua_close(it->second);
            it->second = NULL;
        }
    }
    return 0;
}