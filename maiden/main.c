#include <stdlib.h>
#include <stdio.h>
#include <monome.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

//----- monome led state
unsigned int leds[16][16] = { [0 ... 15][0 ... 15] = 0 };

//----- lua glue
/*
  FIXME: 
  should not compile on the fly for every function call...
  API allows us to push c functions or lua globals directly to stack
  this is just the quickest / least verbose proof of concept
  */

// run an arbitrary chunk of lua
int l_run_code(lua_State* l, const char* code) {
  int err = luaL_loadstring(l, code); // compile code, push to stack
  err |= lua_pcall(l, 0, 0, 0); // pop stack and evaluate
  if(err) {
	printf("lua error : %d, running:\n%s\n", err, code);
  }
  return err;
}

#define CODE_BUF_LEN 1024
static char code_buf[CODE_BUF_LEN];

// call lua functions assigned to handler table in user code
int l_handle_press(lua_State* l, int x, int y) {
  snprintf(code_buf, CODE_BUF_LEN, "handle.press(%d, %d);", x, y);
  l_run_code(l, code_buf);
  /* fixme: should be something like this
	 lua_getglobal(l, "handle");  // push table to stack
	 lua_getField(l, -1, "press"); // push handle.press to stack
	 lua_remove(l, -2); // remove the table
	 lua_pushinteger(l, x); // push args
	 lua_pushinteger(l, y);
	 lua_call(l, 2, 0); // call with 2 args
  */
}

int l_handle_lift(lua_State* l, int x, int y) {
  snprintf(code_buf, CODE_BUF_LEN, "handle.lift(%d, %d);", x, y);
  l_run_code(l, code_buf);
}
		
// set led from lua
int l_set_led(lua_State* l) {
  int n = lua_gettop(l); // num args
}

void l_init(lua_State* l) {
  // register c functions
  lua_register(l, "monome_set_led", &l_set_led);
    // run user init code
  l_run_code(l, "dofile(\"init.lua\");");
}

//----- grid output

void set_led(monome_t *m, int x, int y, int val) {
  leds[x][y] = val;
  monome_led_set(m, x, y, leds[x][y]);
}

//----- grid handlers

void handle_press(const monome_event_t *e, void* p) {
  //  set_led(e->monome, e->grid.x, e->grid.y, 1);
    lua_State *l = (lua_State*)p;
	l_handle_press(l, e->grid.x, e->grid.y);
}
 
void handle_lift(const monome_event_t *e, void* p) {
  //  set_led(e->monome, e->grid.x, e->grid.y, 0);
  lua_State *l = (lua_State*)p;
  l_handle_lift(l, e->grid.x, e->grid.y);
}

//----- main

int main(const char argc, const char** argv) {
  const char *device = "/dev/ttyUSB0";
  monome_t *monome;

  lua_State * lua_S = luaL_newstate();
  luaL_openlibs(lua_S);
  l_init(lua_S);
  
  if( !(monome = monome_open(device, "8000")) ) {
	printf("error opening device %s , exiting\r\n", device);
	return -1;
  }

  printf("successfully opened device at %s \n\n", device);
  
  monome_led_all(monome, 0);
  monome_register_handler(monome, MONOME_BUTTON_DOWN, handle_press, lua_S);
  monome_register_handler(monome, MONOME_BUTTON_UP, handle_lift, lua_S);

  printf("starting event loop... \n\n");
  monome_event_loop(monome);
  monome_close(monome);
}
