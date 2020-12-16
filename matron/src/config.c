#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lauxlib.h>
#include <lualib.h>

#include "lua_eval.h"
#include "hardware/input.h"
#include "hardware/io.h"
#include "hardware/screen.h"

lua_State *config_lvm;

static inline void lua_register_func(lua_State *l, const char* name, lua_CFunction func) {
    lua_pushcfunction(l, func);
    lua_setfield(l, -2, name);
}

static int _add_io(lua_State *l);

int config_init(void) {
    lua_State *l = config_lvm = luaL_newstate();
    luaL_openlibs(l);
    lua_pcall(l, 0, 0, 0);

    lua_newtable(l);

    lua_register_func(l, "add_io", _add_io);

    lua_setglobal(l, "_boot");

    char *home = getenv("HOME");
    char fname[256];
    snprintf(fname, 256, "%s/matronrc.lua", home);
    if (access(fname, R_OK)) {
        fprintf(stderr, "no user matronrc file (%s) found, using default\n", fname);
        snprintf(fname, 256, "%s/norns/matronrc.lua", home);
    }
    fprintf(stderr, "loading matronrc file: %s\n", fname);
    if (l_dofile(l, fname)) {
        fprintf(stderr, "error evaluating matronrc.lua, stop.\n");
        return -1;
    }

    lua_close(l);

    io_setup_all();
    screen_init();

    return 0;
}

void config_deinit(void) {
    io_destroy_all();
}

int _add_io(lua_State *l) {
    lua_check_num_args(2);
    const char *type = luaL_checkstring(l, 1);
    luaL_checktype(l, 2, LUA_TTABLE);

    io_ops_t **ops = io_types;
    while (*ops != NULL) {
        if (strcmp(type, (*ops)->name) == 0) {
            return io_create(l, *ops);
        }
        ops++;
    }

    fprintf(stderr, "ERROR (config) unknown io type: %s\n", type);
    lua_settop(l, 0);
    return luaL_error(l, "unknown input type");
}
