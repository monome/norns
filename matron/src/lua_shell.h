#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct lua_State;

extern void lua_shell_install(lua_State *l);

#ifdef __cplusplus
}
#endif
