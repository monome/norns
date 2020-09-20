#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lauxlib.h>
#include <lualib.h>

#include "lua_eval.h"
#include "hardware/gpio.h"
#include "hardware/screen.h"

lua_State *config_lvm;

static inline void lua_register_func(lua_State *l, const char* name, lua_CFunction func) {
    lua_pushcfunction(l, func);
    lua_setfield(l, -2, name);
}

static int _screen_add(lua_State *l);
static int _input_add(lua_State *l);

int config_init(void) {
    fprintf(stderr, "starting configuration lua vm\n");
    lua_State *l = config_lvm = luaL_newstate();
    luaL_openlibs(l);
    lua_pcall(l, 0, 0, 0);

    lua_newtable(l);

    lua_register_func(l, "screen_add", _screen_add);
    lua_register_func(l, "input_add", _input_add);

    lua_setglobal(l, "_boot");

    char *home = getenv("HOME");
    char cmd[256];
    snprintf(cmd, 256, "%s/norns/matronrc.lua", home);
    if (l_dofile(l, cmd)) {
        return -1;
    }

    lua_close(l);

    screen_init();
    gpio_init();

    return 0;
}

int _screen_add(lua_State *l) {
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
        fprintf(stderr, "unknown screen type for '%s': '%s'\n", name, type_str);
        return luaL_error(l, "unknown screen type");
    }

    lua_pushstring(l, "dev");
    lua_gettable(l, -2);
    if (lua_isstring(l, -1)) {
        const char *dev = lua_tostring(l, -1);
        fprintf(stderr, "screen %s: dev %s\n", name, dev);
        cfg.dev = malloc(strlen(dev) + 1);
        if (!cfg.dev) {
            fprintf(stderr, "ERROR (input) no memory\n");
            lua_settop(l, 0);
            return -1;
        }
        strcpy(cfg.dev, dev);
    } else if (lua_isnil(l, -1)) {
        fprintf(stderr, "screen dev is nil\n");
        cfg.dev = "/dev/fb0";
    } else {
        fprintf(stderr, "ERROR (input) config option 'dev' should be a string\n");
        lua_settop(l, 0);
        return -1;
    }
    lua_pop(l, 1);

    if (screen_create(type, name, &cfg)) {
        fprintf(stderr, "failed to create screen '%s'\n", name);
        lua_settop(l, 0);
        return luaL_error(l, "failed to create screen");
    }

    fprintf(stderr, "create screen %s - %s\n", name, type_str);

    lua_settop(l, 0);
    return 0;
}

int _input_add(lua_State *l) {
    lua_check_num_args(3);
    const char *name = luaL_checkstring(l, 1);
    const char *type_str = luaL_checkstring(l, 2);

    input_type_t type;
    if (strcmp(type_str, "gpio_keys") == 0) {
        type = INPUT_TYPE_GPIO_KEY;
    } else if (strcmp(type_str, "gpio_enc") == 0) {
        type = INPUT_TYPE_GPIO_ENC;
    } else if (strcmp(type_str, "kbm") == 0) {
        type = INPUT_TYPE_KBM;
    } else if (strcmp(type_str, "json") == 0) {
        type = INPUT_TYPE_JSON;
    } else {
        fprintf(stderr, "ERROR (input) unknown input type for '%s': '%s'\n", name, type_str);
        lua_settop(l, 0);
        return luaL_error(l, "unknown input type");
    }

    input_config_t cfg;
    luaL_checktype(l, 3, LUA_TTABLE);    
    
    lua_pushstring(l, "index");
    lua_gettable(l, 3);
    if (lua_isinteger(l, -1)) {
        cfg.index = lua_tointeger(l, -1); 
    } else if (lua_isnil(l, -1)) {
        // no-op
    } else {
        fprintf(stderr, "ERROR (input) config option 'index' should be an integer\n");
        lua_settop(l, 0);
        return -1;
    }
    lua_pop(l, 1);

    lua_pushstring(l, "dev");
    lua_gettable(l, -2);
    if (lua_isstring(l, -1)) {
        const char *dev = lua_tostring(l, -1);
        cfg.dev = malloc(strlen(dev) + 1);
        if (!cfg.dev) {
            fprintf(stderr, "ERROR (input) no memory\n");
            lua_settop(l, 0);
            return -1;
        }
        strcpy(cfg.dev, dev);
    } else if (lua_isnil(l, -1)) {
        // no-op
    } else {
        fprintf(stderr, "ERROR (input) config option 'dev' should be a string\n");
        lua_settop(l, 0);
        return -1;
    }
    lua_pop(l, 1);

    if (input_create(type, name, &cfg)) {
        fprintf(stderr, "failed to create input '%s'\n", name);
        return luaL_error(l, "failed to create input");
    }

    lua_settop(l, 0);
    return 0;
}
