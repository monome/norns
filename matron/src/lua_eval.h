#pragma once

extern int l_dostring(lua_State *L, const char *s, const char *name);

extern int l_report(lua_State *L, int status);

extern int l_docall(lua_State *L, int narg, int nres);

extern int l_handle_line(lua_State *L, char *line);
