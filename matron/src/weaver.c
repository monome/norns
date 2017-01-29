#include <pthread.h>
#include <stdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "events.h"
#include "m.h"
#include "oracle.h"
#include "weaver.h"

lua_State* lvm;

void w_run_code(const char* code) {
  luaL_loadstring(lvm, code); // compile code, push to stack
  lua_pcall(lvm, 0, LUA_MULTRET, 0); // pop stack and evaluate
}

//-------------
//--- declare lua->c glue
static int w_grid_set_led(lua_State* l);
static int w_request_engine_list(lua_State* l);
static int w_load_engine(lua_State* l);

// FIXME: should support dynamically defined OSC format 'engine methods'
// (one fn to request method list, one for varargs OSC)
// for now, hardcode the methods "set parameter" and "load buffer"
static int w_request_buffer_list(lua_State* l);
static int w_load_buffer_name(lua_State* l);
static int w_request_param_list(lua_State* l);
static int w_set_param_name(lua_State* l);

// screen functions
// TODO
// static void w_screen_print(void);
// static extern void w_screen_draw();

void w_init(void) {
  printf("starting lua vm \n");
  lvm = luaL_newstate();
  luaL_openlibs(lvm);
  lua_pcall(lvm, 0, 0, 0);

  // register c functions
  // FIXME: pull these from some kind of descriptor stucture instead
  // not sure how to correctly document lua
  lua_register(lvm, "grid_set_led", &w_grid_set_led);
  
  lua_register(lvm, "list_engines", &w_request_engine_list);
  lua_register(lvm, "load_engine", &w_load_engine);
  
  lua_register(lvm, "list_buffers", &w_request_buffer_list);
  // TODO  lua_register(lvm, "load_buffer", &w_load_buffer_index);
  lua_register(lvm, "load_buffer_name", &w_load_buffer_name);
  
  lua_register(lvm, "list_params", &w_request_param_list);
  lua_register(lvm, "set_param", &w_set_param_name);
  
  // run user init code
  w_run_code("dofile(\"lua/norns.lua\");");
}

//----------------------------------
//---- static definitions

int w_grid_set_led(lua_State* l) {
  int res = 0;
  int x, y, z;
  if(lua_gettop(l) == 3) { // check num args
	if(lua_isnumber(l, 1)) {
	  x = lua_tonumber(l, 1);
	  if(lua_isnumber(l, 2)) {
		y = lua_tonumber(l, 2);
		if(lua_isnumber(l, 3)) {
		  z = lua_tonumber(l, 3);
		  m_grid_set_led(x, y, z);
		}
	  }
	}
  }
  // FIXME(?) : silently ignoring incorrect args...
  return 0;
}

//-- audio processing controls
int w_load_engine(lua_State* l) {
  if(lua_gettop(l) == 1) {
	if(lua_isstring(l, 1)) {
	  o_load_engine(lua_tostring(l, 1));
	}
  }
}

// FIXME: should support dynamically defined OSC formats
int w_load_buffer_name(lua_State* l) {
  if(lua_gettop(l) == 2) {
	if(lua_isstring(l, 1)) {
	  if(lua_isstring(l, 2)) {
		o_load_buffer_name(lua_tostring(l, 1), lua_tostring(l, 2));
	  }
	}
  }
}

int w_set_param_name(lua_State* l) {
  if(lua_gettop(l) == 2) {
	if(lua_isstring(l, 1)) {
	  if(lua_isnumber(l, 2)) {
		o_set_param_name(lua_tostring(l, 1), lua_tonumber(l, 2));
	  }
	}
  }
}

int w_request_engine_list(lua_State* l) {
  printf("engine list request from lvm \n");
  o_request_engine_report();
  // TODO
}

int w_request_buffer_list(lua_State* l) {
  // TODO
}

int w_request_param_list(lua_State* l) {
  // TODO
}


//---- c -> lua glue

//--- hardware input

/*
general form:

void handle_foo(a, b, ... ) { 
  lua_getglobal(l, "handle"); // push table of callbacks to the stack 
  lua_getfield(l, "handle"); // push callback function to stack
  lua_remove(l, -2); // remove the table
  lua_pushinteger(l, a); // push arguments
  lua_pushinteger(l, b); // 
  // ... keep pushing arguments
  lua_call(l, N, 0); // pop stack and call the function with N arguments
 */

/* TODO:
   implement with varargs:
#include <stdio.h>
#include <stdarg.h>

void call_module_function(char* module, char* name, char *fmt, ...)
{
  va_list ap;
  int d;
  char c, *s;
  double f;

  //... put fn on stack as above ...

  va_start(ap, fmt);
  while (*fmt)
	switch (*fmt++) {
	case 's':
	  s = va_arg(ap, char *);
	  // ... push string to stack.. 
	  break;
	case 'd':
	  d = va_arg(ap, int);
	  // ... push int to stack.. 
	  // push int
	  break;
	case 'f':
	  f = va_arg(ap, double);
	  // ... push double to stack ... 
	  break;
	}
	// .. increment arg counter ... 
	va_end(ap);
  }
  /// ... call with count of args ... 
 */

void w_handle_grid_press(int x, int y) {
  lua_getglobal(lvm, "grid");  
  lua_getfield(lvm, -1, "press"); 
  lua_remove(lvm, -2); 
  lua_pushinteger(lvm, x); 
  lua_pushinteger(lvm, y);
  lua_call(lvm, 2, 0); 
}

void w_handle_grid_lift(int x, int y) {
  lua_getglobal(lvm, "grid");  
  lua_getfield(lvm, -1, "lift");
  lua_remove(lvm, -2); 
  lua_pushinteger(lvm, x); 
  lua_pushinteger(lvm, y);
  lua_call(lvm, 2, 0); 
}

void w_handle_stick_axis(int stick, int axis, int value) {
  lua_getglobal(lvm, "joystick");  
  lua_getfield(lvm, -1, "axis"); 
  lua_remove(lvm, -2); 
  lua_pushinteger(lvm, stick); 
  lua_pushinteger(lvm, axis);
  lua_pushinteger(lvm, value);
  lua_call(lvm, 3, 0); 
}

void w_handle_stick_button(int stick, int button, int value) {
  lua_getglobal(lvm, "joystick");  
  lua_getfield(lvm, -1, "button"); 
  lua_remove(lvm, -2); 
  lua_pushinteger(lvm, stick);
  lua_pushinteger(lvm, button);
  lua_pushinteger(lvm, value);
  lua_call(lvm, 3, 0); 
}
