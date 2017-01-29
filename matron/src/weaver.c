#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "weaver.h"

lua_State* lua;

void w_init(void) {
  lua = luaL_newstate();
  luaL_openlibs(lua);
  lua_pcall(lua, 0, 0, 0);
}
void w_run_code(const char* code) {
  luaL_loadstring(lua, code); // compile code, push to stack
  lua_pcall(lua, 0, LUA_MULTRET, 0); // pop stack and evaluate
}
