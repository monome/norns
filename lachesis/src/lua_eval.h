#pragma once

#include <lua.h>

#define STRING_NUM(n) #n
#define LUA_ARG_ERROR(n) "error: requires " STRING_NUM(n) " arguments"
#define lua_check_num_args(n)                   \
    if (lua_gettop(l) != n) {                   \
        return luaL_error(l, LUA_ARG_ERROR(n)); \
    }

extern int l_dostring(lua_State *L, const char *s, const char *name);

extern int l_report(lua_State *L, int status);

extern int l_docall(lua_State *L, int narg, int nres);

extern int l_dofile(lua_State *L, const char* filename);

extern int l_handle_line(lua_State *L, char *line);
