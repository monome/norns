// tests for matron/src/lua_shell.cc.
//
// these drive a real lua vm.

#include <doctest/doctest.h>
#include <string>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "lua_shell.h"

#include "lua_shell_stubs.h"

namespace {

struct vm_t {
    vm_t() {
        lua_shell_stubs_reset();
        l = luaL_newstate();
        luaL_openlibs(l);
        lua_shell_install(l);
        run("printed = {}\nprint = function(s) printed[#printed + 1] = s end");
    }

    ~vm_t() {
        lua_close(l);
    }

    void run(const char *src) {
        if (luaL_dostring(l, src) != LUA_OK) {
            FAIL(lua_tostring(l, -1));
        }
    }

    void push_eval(const char *expr) {
        const std::string src = std::string("return ") + expr;
        if (luaL_dostring(l, src.c_str()) != LUA_OK) {
            FAIL(lua_tostring(l, -1));
        }
    }

    bool is_nil(const char *expr) {
        push_eval(expr);
        const bool v = lua_isnil(l, -1);
        lua_pop(l, 1);
        return v;
    }

    bool boolean(const char *expr) {
        push_eval(expr);
        const bool v = lua_toboolean(l, -1);
        lua_pop(l, 1);
        return v;
    }

    std::string string(const char *expr) {
        push_eval(expr);
        const char *s = lua_tostring(l, -1);
        const std::string v = s != nullptr ? s : "";
        lua_pop(l, 1);
        return v;
    }

    lua_Integer integer(const char *expr) {
        push_eval(expr);
        const lua_Integer v = lua_tointeger(l, -1);
        lua_pop(l, 1);
        return v;
    }

    lua_State *l;
};

} // namespace

//---------------------------------
//--- the stdlib contract

TEST_CASE("lua_shell: a command that exits zero returns the stdlib triple") {
    vm_t vm;
    vm.run("ok, how, code = os.execute('true')");

    CHECK(vm.boolean("ok"));
    CHECK(vm.string("how") == "exit");
    CHECK(vm.integer("code") == 0);
}

TEST_CASE("lua_shell: a non-zero exit returns nil and its own code") {
    vm_t vm;
    g_stub_result = SIDECAR_CMD_FAILED;
    g_stub_code = 7;
    vm.run("ok, how, code = os.execute('exit 7')");

    CHECK(vm.is_nil("ok"));
    CHECK(vm.string("how") == "exit");
    CHECK(vm.integer("code") == 7);
}

TEST_CASE("lua_shell: a signalled command reports the signal, not an exit") {
    vm_t vm;
    g_stub_result = SIDECAR_CMD_FAILED;
    g_stub_signalled = true;
    g_stub_code = 9;
    vm.run("ok, how, code = os.execute('kill -9 $$')");

    CHECK(vm.is_nil("ok"));
    CHECK(vm.string("how") == "signal");
    CHECK(vm.integer("code") == 9);
}

TEST_CASE("lua_shell: a command that never ran reads as a plain failure") {
    vm_t vm;
    g_stub_result = SIDECAR_CMD_FAILED;
    g_stub_code = 0;
    vm.run("ok, how, code = os.execute('x')");

    CHECK(vm.is_nil("ok"));
    CHECK(vm.string("how") == "exit");
    CHECK(vm.integer("code") == 1);
}

TEST_CASE("lua_shell: a transport failure reads as a failure too") {
    vm_t vm;
    g_stub_result = SIDECAR_TRANSPORT_FAILED;
    vm.run("ok, how, code = os.execute('x')");

    CHECK(vm.is_nil("ok"));
    CHECK(vm.string("how") == "exit");
    CHECK(vm.integer("code") == 1);
}

TEST_CASE("lua_shell: os.execute with no argument reports a shell is available") {
    vm_t vm;
    vm.run("available = os.execute()");

    CHECK(vm.boolean("available"));
    CHECK(g_call_count == 0);
}

//---------------------------------
//--- output

TEST_CASE("lua_shell: what the command printed reaches print") {
    vm_t vm;
    g_stub_output = "hello\n";
    vm.run("os.execute('echo hello')");

    CHECK(vm.integer("#printed") == 1);
    CHECK(vm.string("printed[1]") == "hello");
}

TEST_CASE("lua_shell: the command's own trailing newline is not doubled") {
    vm_t vm;
    g_stub_output = "a\nb\n";
    vm.run("os.execute('x')");

    CHECK(vm.string("printed[1]") == "a\nb");
}

TEST_CASE("lua_shell: a command printing a bare newline still prints a line") {
    vm_t vm;
    g_stub_output = "\n";
    vm.run("os.execute('echo')");

    CHECK(vm.integer("#printed") == 1);
    CHECK(vm.string("printed[1]") == "");
}

TEST_CASE("lua_shell: a silent command prints nothing at all") {
    vm_t vm;
    g_stub_output = "";
    vm.run("os.execute('true')");

    CHECK(vm.integer("#printed") == 0);
}

TEST_CASE("lua_shell: output survives a failing command") {
    vm_t vm;
    g_stub_output = "partial\n";
    g_stub_result = SIDECAR_CMD_FAILED;
    g_stub_code = 1;
    vm.run("ok = os.execute('x')");

    CHECK(vm.is_nil("ok"));
    CHECK(vm.string("printed[1]") == "partial");
}

TEST_CASE("lua_shell: a script that breaks print does not break os.execute") {
    vm_t vm;
    g_stub_output = "hi\n";
    vm.run("print = function() error('no printing here') end");
    vm.run("ok, how, code = os.execute('x')");

    CHECK(vm.boolean("ok"));
    CHECK(vm.integer("code") == 0);
}

//---------------------------------
//--- what reaches the sidecar

TEST_CASE("lua_shell: the command text goes to the sidecar unchanged") {
    vm_t vm;
    vm.run("os.execute('ls -la /tmp')");

    REQUIRE(g_last_cmd != nullptr);
    CHECK(std::string(g_last_cmd) == "ls -la /tmp");
    CHECK(g_call_count == 1);
}

TEST_CASE("lua_shell: os.execute asks for no silence ceiling") {
    vm_t vm;
    vm.run("os.execute('sleep 300')");

    CHECK(g_last_timeout_ms == SIDECAR_CMD_TIMEOUT_NONE);
}

TEST_CASE("lua_shell: a non-string command is refused rather than run") {
    vm_t vm;
    vm.run("ok = pcall(os.execute, {})");

    CHECK_FALSE(vm.boolean("ok"));
    CHECK(g_call_count == 0);
}

//---------------------------------
//--- io.popen

TEST_CASE("lua_shell: io.popen reads a command's whole output") {
    vm_t vm;
    g_stub_output = "one\ntwo\n";
    vm.run("f = io.popen('ls')\nall = f:read('a')");

    CHECK(vm.string("all") == "one\ntwo\n");
}

TEST_CASE("lua_shell: io.popen accepts the older starred read formats") {
    vm_t vm;
    g_stub_output = "hi\n";
    vm.run("f = io.popen('x')\nall = f:read('*a')");

    CHECK(vm.string("all") == "hi\n");
}

TEST_CASE("lua_shell: reading lines drops the newline, and runs out at the end") {
    vm_t vm;
    g_stub_output = "one\ntwo\n";
    vm.run("f = io.popen('x')\na = f:read('l')\nb = f:read('l')\nc = f:read('l')");

    CHECK(vm.string("a") == "one");
    CHECK(vm.string("b") == "two");
    CHECK(vm.is_nil("c"));
}

TEST_CASE("lua_shell: the L format keeps the newline") {
    vm_t vm;
    g_stub_output = "one\ntwo";
    vm.run("f = io.popen('x')\na = f:read('L')\nb = f:read('L')");

    CHECK(vm.string("a") == "one\n");
    CHECK(vm.string("b") == "two"); // the last line had none to keep
}

TEST_CASE("lua_shell: reading with no format returns a line") {
    vm_t vm;
    g_stub_output = "only\n";
    vm.run("f = io.popen('x')\na = f:read()");

    CHECK(vm.string("a") == "only");
}

TEST_CASE("lua_shell: a byte count reads exactly that many") {
    vm_t vm;
    g_stub_output = "abcdef";
    vm.run("f = io.popen('x')\na = f:read(3)\nb = f:read(3)\nc = f:read(1)");

    CHECK(vm.string("a") == "abc");
    CHECK(vm.string("b") == "def");
    CHECK(vm.is_nil("c"));
}

TEST_CASE("lua_shell: reading zero bytes probes for the end of the output") {
    vm_t vm;
    g_stub_output = "abc";
    vm.run("f = io.popen('x')\na = f:read(0)\nf:read('a')\nb = f:read(0)");

    CHECK(vm.boolean("a == ''"));
    CHECK(vm.is_nil("b"));
}

TEST_CASE("lua_shell: reading a number parses it out of the output") {
    vm_t vm;
    g_stub_output = "  42  7\n";
    vm.run("f = io.popen('x')\na = f:read('n')\nb = f:read('n')");

    CHECK(vm.integer("a") == 42);
    CHECK(vm.integer("b") == 7);
}

TEST_CASE("lua_shell: several formats can be read at once") {
    vm_t vm;
    g_stub_output = "one\ntwo\n";
    vm.run("f = io.popen('x')\na, b = f:read('l', 'l')");

    CHECK(vm.string("a") == "one");
    CHECK(vm.string("b") == "two");
}

TEST_CASE("lua_shell: reading past the end yields an empty string for a, nil for a line") {
    vm_t vm;
    g_stub_output = "";
    vm.run("f = io.popen('x')\na = f:read('a')\nb = f:read('l')");

    CHECK(vm.string("a") == "");
    CHECK(vm.is_nil("b"));
}

TEST_CASE("lua_shell: lines iterates the output") {
    vm_t vm;
    g_stub_output = "one\ntwo\nthree\n";
    vm.run("f = io.popen('x')\ngot = {}\nfor line in f:lines() do got[#got + 1] = line end");

    CHECK(vm.integer("#got") == 3);
    CHECK(vm.string("got[1]") == "one");
    CHECK(vm.string("got[3]") == "three");
}

TEST_CASE("lua_shell: close reports how the command ended") {
    vm_t vm;
    vm.run("f = io.popen('true')\nok, how, code = f:close()");

    CHECK(vm.boolean("ok"));
    CHECK(vm.string("how") == "exit");
    CHECK(vm.integer("code") == 0);
}

TEST_CASE("lua_shell: close reports a failing command's own code") {
    vm_t vm;
    g_stub_result = SIDECAR_CMD_FAILED;
    g_stub_code = 3;
    vm.run("f = io.popen('x')\nok, how, code = f:close()");

    CHECK(vm.is_nil("ok"));
    CHECK(vm.string("how") == "exit");
    CHECK(vm.integer("code") == 3);
}

TEST_CASE("lua_shell: output is still readable after a failing command") {
    vm_t vm;
    g_stub_output = "partial\n";
    g_stub_result = SIDECAR_CMD_FAILED;
    g_stub_code = 1;
    vm.run("f = io.popen('x')\nall = f:read('a')");

    CHECK(vm.string("all") == "partial\n");
}

TEST_CASE("lua_shell: reading a closed handle is an error, as it is in the stdlib") {
    vm_t vm;
    vm.run("f = io.popen('x')\nf:close()\nok = pcall(function() return f:read('a') end)");

    CHECK_FALSE(vm.boolean("ok"));
}

TEST_CASE("lua_shell: closing twice is an error rather than a second answer") {
    vm_t vm;
    vm.run("f = io.popen('x')\nf:close()\nok = pcall(function() return f:close() end)");

    CHECK_FALSE(vm.boolean("ok"));
}

TEST_CASE("lua_shell: iterating lines after close is an error too") {
    vm_t vm;
    g_stub_output = "one\ntwo\n";
    vm.run("f = io.popen('x')\nit = f:lines()\nf:close()\nok = pcall(it)");

    CHECK_FALSE(vm.boolean("ok"));
}

TEST_CASE("lua_shell: write mode is refused") {
    // remove this test when io.popen is fixed
    vm_t vm;
    vm.run("ok, err = pcall(io.popen, 'x', 'w')");

    CHECK_FALSE(vm.boolean("ok"));
    CHECK(vm.string("err").find("write mode") != std::string::npos);
    CHECK(g_call_count == 0);
}

TEST_CASE("lua_shell: an unknown mode is refused") {
    vm_t vm;
    vm.run("ok, err = pcall(io.popen, 'x', 'q')");

    CHECK_FALSE(vm.boolean("ok"));
    CHECK(vm.string("err").find("write mode") == std::string::npos);
    CHECK(g_call_count == 0);
}

TEST_CASE("lua_shell: read mode may be given explicitly") {
    vm_t vm;
    g_stub_output = "fine\n";
    vm.run("f = io.popen('x', 'r')\nall = f:read('a')");

    CHECK(vm.string("all") == "fine\n");
}

TEST_CASE("lua_shell: io.popen asks for no silence ceiling either") {
    vm_t vm;
    vm.run("f = io.popen('sleep 300')");

    CHECK(g_last_timeout_ms == SIDECAR_CMD_TIMEOUT_NONE);
    CHECK(g_call_count == 1);
}

TEST_CASE("lua_shell: a transport failure still yields a usable handle") {
    vm_t vm;
    g_stub_result = SIDECAR_TRANSPORT_FAILED;
    vm.run("f = io.popen('x')\nall = f:read('a')\nok, how, code = f:close()");

    CHECK(vm.string("all") == "");
    CHECK(vm.is_nil("ok"));
    CHECK(vm.integer("code") == 1);
}
