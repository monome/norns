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

void handle_lua_error(int val) {
  switch(val) {
  case LUA_ERRRUN:
	printf("[lua runtime error!] \n");
	break;
  case LUA_ERRMEM:
	printf("[lua memory allocation error!]\n");
	break;
  case LUA_ERRGCMM:
	printf("[lua __gc metamethod error!]\n");
	break;
  case LUA_OK:
  default:
	;; // nothing to do
  }

}

void w_run_code(const char* code) {
  luaL_loadstring(lvm, code); // compile code, push to stack
  int ret = lua_pcall(lvm, 0, LUA_MULTRET, 0); // pop stack and evaluate
  if(ret) { handle_lua_error(ret); }
}

//-------------
//--- declare lua->c glue
static int w_grid_set_led(lua_State* l);
static int w_request_engine_report(lua_State* l);
static int w_load_engine(lua_State* l);

// FIXME: should support dynamically defined OSC format 'engine methods'
// (one fn to request method list, one for varargs OSC)
// for now, hardcode methods for "buffers" and "parameters"
static int w_request_buffer_report(lua_State* l);
static int w_load_buffer_name(lua_State* l);
static int w_request_param_report(lua_State* l);
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
  
  lua_register(lvm, "report_engines", &w_request_engine_report);
  lua_register(lvm, "load_engine", &w_load_engine);
  
  lua_register(lvm, "report_buffers", &w_request_buffer_report);
  lua_register(lvm, "load_buffer", &w_load_buffer_name);
  // TODO  lua_register(lvm, "load_buffer_idx", &w_load_buffer_index);
  
  lua_register(lvm, "report_params", &w_request_param_report);
  lua_register(lvm, "set_param", &w_set_param_name);
  // TODO  lua_register(lvm, "load_param_idx", &w_load_param_index);
  
  // run system init code
  w_run_code("dofile(\"lua/norns.lua\");");
}

// run user startup code
// audio backend should be running
void w_user_startup(void) {
  lua_getglobal(lvm, "startup");
  lua_pcall(lvm, 0, 0, 0);
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

int w_request_engine_report(lua_State* l) {
  o_request_engine_report();
}

int w_request_buffer_report(lua_State* l) {
  o_request_buffer_report();
}

int w_request_param_report(lua_State* l) {
  o_request_param_report();
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

// helper for calling grid handlers
static inline void
w_call_grid_handler(const char* name, int x, int y) {
  lua_getglobal(lvm, "grid");  
  lua_getfield(lvm, -1, name); 
  lua_remove(lvm, -2); 
  lua_pushinteger(lvm, x); 
  lua_pushinteger(lvm, y);
  int ret = lua_pcall(lvm, 2, 0, 0);
  if(ret) { handle_lua_error(ret); }

}
void w_handle_grid_press(int x, int y) {
  w_call_grid_handler("press", x, y);
}

void w_handle_grid_lift(int x, int y) {
  w_call_grid_handler("lift", x, y);
}

// helper for calling joystick handlers
static inline void
w_call_stick_handler(const char* name, int stick, int what, int val) {
  lua_getglobal(lvm, "joystick");  
  lua_getfield(lvm, -1, name); 
  lua_remove(lvm, -2); 
  lua_pushinteger(lvm, stick); 
  lua_pushinteger(lvm, what);
  lua_pushinteger(lvm, val);
  int ret = lua_pcall(lvm, 3, 0, 0);
  if(ret) { handle_lua_error(ret); }
}

void w_handle_stick_axis(int stick, int axis, int value) {
  w_call_stick_handler("axis", stick+1, axis+1, value);
}

void w_handle_stick_button(int stick, int button, int value) {
  w_call_stick_handler("button", stick+1, button+1, value);
}

// helper for pushing array of c strings
static inline void
w_push_string_array(const char** arr, const int n) {
  // allocate and push the table
  lua_createtable(lvm, n, 0);
  // set each entry
  for (int i=0; i<n; i++) {
    lua_pushstring(lvm, arr[i]);
    lua_rawseti(lvm, -2, i+1);
  }
  // push count of entries
  lua_pushinteger(lvm, n);
}

// helper for calling report handlers
static inline void
w_call_report_handler(const char* name, const char** arr, const int num) {
  lua_getglobal(lvm, "report");  
  lua_getfield(lvm, -1, name); 
  lua_remove(lvm, -2); 
  w_push_string_array(arr, num);
  int ret = lua_pcall(lvm, 2, 0, 0);
  if(ret) { handle_lua_error(ret); }

}

// audio engine report handlers
void w_handle_buffer_report(const char** arr, const int num) {
  w_call_report_handler( "buffer", arr, num);
}

void w_handle_engine_report(const char** arr, const int num) {
  w_call_report_handler("engine", arr, num);
}

void w_handle_param_report(const char** arr, const int num) {
  w_call_report_handler("param", arr, num);
}
