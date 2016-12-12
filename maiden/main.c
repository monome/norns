#include <stdlib.h>
#include <stdio.h>
#include <monome.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

//----- monome led state
unsigned int m_leds[16][16] = { [0 ... 15][0 ... 15] = 0 };
// FIXME: make this private somewhere
monome_t *monome;

//----- grid output

void m_set_led(int x, int y, int val) {
  m_leds[x][y] = val;
  monome_led_set(monome, x, y, m_leds[x][y]);
}

//----- lua glue

// compile and run an arbitrary chunk of lua code
int l_run_code(lua_State* l, const char* code) {
  int err = luaL_loadstring(l, code); // compile code, push to stack
  err |= lua_pcall(l, 0, 0, 0); // pop stack and evaluate
  if(err) {
	printf("lua error : %d, running:\n%s\n", err, code);
  }
  return err;
}

// call lua functions assigned to handler table in user code
int l_handle_press(lua_State* l, int x, int y) {
  lua_getglobal(l, "handle");  // push table to stack
  lua_getfield(l, -1, "press"); // push handle.press to stack
  lua_remove(l, -2); // remove the table
  lua_pushinteger(l, x); // push args
  lua_pushinteger(l, y);
  lua_call(l, 2, 0); // call with 2 args
  
}

int l_handle_lift(lua_State* l, int x, int y) {
  lua_getglobal(l, "handle");  // push table to stack
  lua_getfield(l, -1, "lift"); // push handle.press to stack
  lua_remove(l, -2); // remove the table
  lua_pushinteger(l, x); // push args
  lua_pushinteger(l, y);
  lua_call(l, 2, 0); // call with 2 args

}
		
// set led from lua
int l_set_led(lua_State* l) {
  int res = 0;
  int x, y, z;
  if(lua_gettop(l) == 3) { // check num args
	if(lua_isnumber(l, 1)) {
	  x = lua_tonumber(l, 1);
	  if(lua_isnumber(l, 2)) {
		y = lua_tonumber(l, 2);
		if(lua_isnumber(l, 3)) {
		  z = lua_tonumber(l, 3);
		  m_set_led(x, y, z);
		}
	  }
	}
  }
  // FIXME(?) : silently ignoring incorrect args...
  return 0;
}

void l_init(lua_State* l) {
  // register c functions
  lua_register(l, "monome_set_led", &l_set_led);
    // run user init code
  l_run_code(l, "dofile(\"init.lua\");");
}

//----- grid handlers

void m_handle_press(const monome_event_t *e, void* p) {
  //  set_led(e->monome, e->grid.x, e->grid.y, 1);
    lua_State *l = (lua_State*)p;
	l_handle_press(l, e->grid.x, e->grid.y);
}
 
void m_handle_lift(const monome_event_t *e, void* p) {
  //  set_led(e->monome, e->grid.x, e->grid.y, 0);
  lua_State *l = (lua_State*)p;
  l_handle_lift(l, e->grid.x, e->grid.y);
}

//----- main

int main(const char argc, const char** argv) {
  const char *device = "/dev/ttyUSB0";

  lua_State * lua_S = luaL_newstate();
  luaL_openlibs(lua_S);
  l_init(lua_S);
  
  if( !(monome = monome_open(device, "8000")) ) {
	printf("error opening device %s , exiting\r\n", device);
	return -1;
  }

  printf("successfully opened device at %s \n\n", device);
  
  monome_led_all(monome, 0);
  monome_register_handler(monome, MONOME_BUTTON_DOWN, m_handle_press, lua_S);
  monome_register_handler(monome, MONOME_BUTTON_UP, m_handle_lift, lua_S);

  printf("starting event loop... \n\n");
  monome_event_loop(monome);
  monome_close(monome);
}
