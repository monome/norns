// os.execute and io.popen run through the sidecar.

// FIXME io.popen hands back a finished command rather than a live process, so
// reads do not stream and an unbounded producer never returns. not fixable at
// this layer, since the transport runs one command at a time.

#include <stdlib.h>
#include <string.h>

#include <lauxlib.h>
#include <lua.h>

#include "system_cmd.h"

#include "lua_shell.h"

#define POPEN_HANDLE "norns.popen"

static void print_capture(lua_State *l, const char *text) {
    if (text == NULL || text[0] == '\0') {
        return;
    }

    size_t len = strlen(text);
    // print adds its own newline
    if (text[len - 1] == '\n') {
        len -= 1;
    }

    lua_getglobal(l, "print");
    if (!lua_isfunction(l, -1)) {
        lua_pop(l, 1);
        return;
    }

    lua_pushlstring(l, text, len);
    if (lua_pcall(l, 1, 0, 0) != LUA_OK) {
        lua_pop(l, 1);
    }
}

static int push_result(lua_State *l, const sidecar_status_t *status) {
    if (status->result == SIDECAR_OK) {
        lua_pushboolean(l, 1);
        lua_pushliteral(l, "exit");
        lua_pushinteger(l, 0);
        return 3;
    }

    const bool ran = status->signalled || status->code != 0;

    lua_pushnil(l);
    lua_pushstring(l, ran && status->signalled ? "signal" : "exit");
    lua_pushinteger(l, ran ? status->code : 1);
    return 3;
}

static int _os_execute(lua_State *l) {
    if (lua_isnoneornil(l, 1)) {
        lua_pushboolean(l, 1);
        return 1;
    }

    const char *cmd = luaL_checkstring(l, 1);

    char *capture = NULL;
    size_t size = 0;
    sidecar_status_t status = {};
    system_cmd_sync(cmd, &capture, &size, &status, SIDECAR_CMD_TIMEOUT_NONE);

    print_capture(l, capture);
    free(capture);

    return push_result(l, &status);
}

typedef struct {
    char *buf;
    size_t len;
    size_t pos;
    sidecar_status_t status;
    bool closed;
} popen_handle_t;

static popen_handle_t *check_open_handle(lua_State *l) {
    popen_handle_t *h = (popen_handle_t *)luaL_checkudata(l, 1, POPEN_HANDLE);
    if (h->closed) {
        luaL_error(l, "attempt to use a closed file");
    }
    return h;
}

static size_t remaining(const popen_handle_t *h) {
    return h->len - h->pos;
}

static void read_line(lua_State *l, popen_handle_t *h, bool keep_newline) {
    if (remaining(h) == 0) {
        lua_pushnil(l);
        return;
    }

    const char *from = h->buf + h->pos;
    const char *nl = (const char *)memchr(from, '\n', remaining(h));
    const size_t taken = nl != NULL ? (size_t)(nl - from) + 1 : remaining(h);
    const size_t pushed = (nl != NULL && !keep_newline) ? taken - 1 : taken;

    lua_pushlstring(l, from, pushed);
    h->pos += taken;
}

static void read_count(lua_State *l, popen_handle_t *h, size_t want) {
    if (want == 0) {
        if (remaining(h) == 0) {
            lua_pushnil(l);
        } else {
            lua_pushliteral(l, "");
        }
        return;
    }

    if (remaining(h) == 0) {
        lua_pushnil(l);
        return;
    }

    const size_t taken = want < remaining(h) ? want : remaining(h);
    lua_pushlstring(l, h->buf + h->pos, taken);
    h->pos += taken;
}

static void read_number(lua_State *l, popen_handle_t *h) {
    const char *from = h->buf + h->pos;
    char *end = NULL;
    const double v = strtod(from, &end);
    if (end == from) {
        lua_pushnil(l);
        return;
    }
    lua_pushnumber(l, (lua_Number)v);
    h->pos += (size_t)(end - from);
}

static void read_format(lua_State *l, popen_handle_t *h, int index) {
    if (lua_isnumber(l, index)) {
        read_count(l, h, (size_t)lua_tointeger(l, index));
        return;
    }

    const char *fmt = luaL_checkstring(l, index);
    if (*fmt == '*') {
        fmt++;
    }

    switch (*fmt) {
    case 'a':
        lua_pushlstring(l, h->buf + h->pos, remaining(h));
        h->pos = h->len;
        break;
    case 'l':
        read_line(l, h, false);
        break;
    case 'L':
        read_line(l, h, true);
        break;
    case 'n':
        read_number(l, h);
        break;
    default:
        luaL_argerror(l, index, "invalid format");
    }
}

static int _popen_read(lua_State *l) {
    popen_handle_t *h = check_open_handle(l);

    const int args = lua_gettop(l) - 1;
    if (args == 0) {
        read_line(l, h, false);
        return 1;
    }

    for (int i = 0; i < args; ++i) {
        read_format(l, h, i + 2);
    }
    return args;
}

static int _popen_lines_iter(lua_State *l) {
    popen_handle_t *h = (popen_handle_t *)lua_touserdata(l, lua_upvalueindex(1));
    if (h->closed) {
        return luaL_error(l, "attempt to use a closed file");
    }
    read_line(l, h, false);
    return 1;
}

static int _popen_lines(lua_State *l) {
    check_open_handle(l);
    lua_pushvalue(l, 1);
    lua_pushcclosure(l, _popen_lines_iter, 1);
    return 1;
}

static void handle_release(popen_handle_t *h) {
    free(h->buf);
    h->buf = NULL;
    h->len = 0;
    h->pos = 0;
}

static int _popen_close(lua_State *l) {
    popen_handle_t *h = check_open_handle(l);
    h->closed = true;
    handle_release(h);
    return push_result(l, &h->status);
}

static int _popen_gc(lua_State *l) {
    popen_handle_t *h = (popen_handle_t *)luaL_checkudata(l, 1, POPEN_HANDLE);
    handle_release(h);
    return 0;
}

static int _popen_tostring(lua_State *l) {
    popen_handle_t *h = (popen_handle_t *)luaL_checkudata(l, 1, POPEN_HANDLE);
    lua_pushfstring(l, h->closed ? "file (closed)" : "file (%p)", (void *)h);
    return 1;
}

static int _io_popen(lua_State *l) {
    const char *cmd = luaL_checkstring(l, 1);
    const char *mode = luaL_optstring(l, 2, "r");

    // fail on "w" mode, there is no live process to write to
    // see the FIXME at the top of this file
    if (strcmp(mode, "w") == 0) {
        return luaL_error(l, "io.popen: write mode is not supported on norns at this time.");
    }

    if (strcmp(mode, "r") != 0) {
        return luaL_error(l, "io.popen: mode must be 'r' or 'w', got '%s'", mode);
    }

    char *capture = NULL;
    size_t size = 0;
    sidecar_status_t status = {};
    system_cmd_sync(cmd, &capture, &size, &status, SIDECAR_CMD_TIMEOUT_NONE);

    popen_handle_t *h = (popen_handle_t *)lua_newuserdata(l, sizeof(popen_handle_t));
    h->buf = capture;
    // size counts the terminator, which is not part of the output
    h->len = capture != NULL && size > 0 ? size - 1 : 0;
    h->pos = 0;
    h->status = status;
    h->status.err = NULL;
    h->closed = false;

    luaL_getmetatable(l, POPEN_HANDLE);
    lua_setmetatable(l, -2);
    return 1;
}

static void register_popen_handle(lua_State *l) {
    static const luaL_Reg methods[] = {
        {"read", _popen_read},
        {"lines", _popen_lines},
        {"close", _popen_close},
        {NULL, NULL},
    };

    luaL_newmetatable(l, POPEN_HANDLE);

    lua_pushcfunction(l, _popen_gc);
    lua_setfield(l, -2, "__gc");
    lua_pushcfunction(l, _popen_tostring);
    lua_setfield(l, -2, "__tostring");

    lua_newtable(l);
    luaL_setfuncs(l, methods, 0);
    lua_setfield(l, -2, "__index");

    lua_pop(l, 1);
}

void lua_shell_install(lua_State *l) {
    register_popen_handle(l);

    lua_getglobal(l, "os");
    if (lua_istable(l, -1)) {
        lua_pushcfunction(l, _os_execute);
        lua_setfield(l, -2, "execute");
    }
    lua_pop(l, 1);

    lua_getglobal(l, "io");
    if (lua_istable(l, -1)) {
        lua_pushcfunction(l, _io_popen);
        lua_setfield(l, -2, "popen");
    }
    lua_pop(l, 1);
}
