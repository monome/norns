#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lauxlib.h>
#include <lualib.h>

#include "lua_eval.h"
#include "hardware/screen.h"

lua_State *config_lvm;

static inline void lua_register_func(lua_State *l, const char* name, lua_CFunction func) {
    lua_pushcfunction(l, func);
    lua_setfield(l, -2, name);
}

static int _screen_create(lua_State *l);

int config_init(void) {
    fprintf(stderr, "starting configuration lua vm\n");
    lua_State *l = config_lvm = luaL_newstate();
    luaL_openlibs(l);
    lua_pcall(l, 0, 0, 0);

    lua_newtable(l);

    lua_register_func(l, "screen_create", _screen_create);

    lua_setglobal(l, "_boot");

    char *home = getenv("HOME");
    char cmd[256];

    snprintf(cmd, 256, "%s/norns/matronrc.lua", home);
    if (l_dofile(l, cmd)) {
        return -1;
    }

    screen_init();
    return 0;
}

int _screen_create(lua_State *l) {
    lua_check_num_args(3);
    const char *name = luaL_checkstring(l, 1);
    const char *type_str = luaL_checkstring(l, 2);
    screen_config_t cfg;
    luaL_checktype(l, 3, LUA_TTABLE);

    screen_type_t type;
    if (strcmp(type_str, "fbdev") == 0) {
        type = SCREEN_TYPE_FBDEV;
    } else if (strcmp(type_str, "sdl") == 0) {
        type = SCREEN_TYPE_SDL;
    } else if (strcmp(type_str, "json") == 0) {
        type = SCREEN_TYPE_JSON;
    } else {
        fprintf(stderr, "unknown screen type: '%s'\n", name);
        return luaL_error(l, "unknown screen type");
    }

    if (screen_create(type, name, &cfg)) {
        return luaL_error(l, "failed to create screen");
    }

    return 0;
}
