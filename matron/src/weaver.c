/*
 * weaver.c
 *
 * c->lua and lua->c interface
 *
 */

// standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// linux / posix
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

// lua
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

// norns
#include "device_input.h"
#include "device_monome.h"
#include "events.h"
#include "lua_eval.h"
#include "m.h"
#include "timers.h"
#include "oracle.h"
#include "weaver.h"

//------
//---- global lua state!
lua_State *lvm;

void w_run_code(const char *code) {
  l_dostring(lvm, code, "w_run_code");
  fflush(stdout);
}

void w_handle_line(char *line) {
  l_handle_line(lvm, line);
}

//-------------
//--- declare lua->c glue

// grid
static int w_grid_set_led(lua_State *l);
static int w_grid_refresh(lua_State *l);
// audio engine
static int w_request_engine_report(lua_State *l);
static int w_load_engine(lua_State *l);
static int w_request_command_report(lua_State *l);
static int w_send_command(lua_State *l);
// timers
static int w_timer_start(lua_State *l);
static int w_timer_stop(lua_State *l);
// request current time since Epoch
static int w_get_time(lua_State *l);

// screen functions
// TODO
// static void w_screen_print(void);
// static extern void w_screen_draw();
//... ?

// boilerplate: push a function to the stack, from field in global 'norns'
static inline void
w_push_norns_func(const char *field, const char *func) {
  lua_getglobal(lvm, "norns");
  lua_getfield(lvm, -1, field);
  lua_remove(lvm, -2);
  lua_getfield(lvm, -1, func);
  lua_remove(lvm, -2);
}

void w_init(void) {
  printf("starting lua vm \n");
  lvm = luaL_newstate();
  luaL_openlibs(lvm);
  lua_pcall(lvm, 0, 0, 0);
  fflush(stdout);

  // FIXME: how/where to document these in lua
  lua_register(lvm, "grid_set_led", &w_grid_set_led);
  lua_register(lvm, "grid_refresh", &w_grid_refresh);

  lua_register(lvm, "report_engines", &w_request_engine_report);
  lua_register(lvm, "load_engine", &w_load_engine);

  lua_register(lvm, "report_commands", &w_request_command_report);
  lua_register(lvm, "send_command", &w_send_command);

  lua_register(lvm, "timer_start", &w_timer_start);
  lua_register(lvm, "timer_stop", &w_timer_stop);

  lua_register(lvm, "get_time", &w_get_time);

  // run system init code
  char *config = getenv("NORNS_CONFIG");
  char *home = getenv("HOME");
  char cmd[256];

  if(config == NULL) {
    snprintf(cmd, 256, "dofile('%s/norns/lua/config.lua')\n", home);
  } else {
    snprintf(cmd, 256, "dofile('%s')\n", config);
  }
  printf("running lua config file: %s", cmd);
  w_run_code(cmd);
  w_run_code("require('norns')");
}

// run user startup code
// audio backend should be running
void w_user_startup(void) {
  lua_getglobal(lvm, "startup");
  l_report( lvm, l_docall(lvm, 0, 0) );
}

void w_deinit(void) {
  // FIXME: lua is leaking memory. doesn't really matter
}

//----------------------------------
//---- static definitions

int w_grid_set_led(lua_State *l) {
  struct dev_monome *md;
  int x, y, z;
  if(lua_gettop(l) != 4) { // check num args
    goto args_error;
  }
  if( lua_islightuserdata(l, 1) ) {
    md = lua_touserdata(l, 1);
  } else {
    goto args_error;
  }

  if( lua_isnumber(l, 2) ) {
    x = lua_tonumber(l, 2) - 1; // convert from 1-base
  } else {
    goto args_error;
  }

  if( lua_isnumber(l, 3) ) {
    y = lua_tonumber(l, 3) - 1; // convert from 1-base
  } else {
    goto args_error;
  }
  if( lua_isnumber(l, 4) ) {
    z = lua_tonumber(l, 4); // don't convert value!
  } else {
    goto args_error;
  }

  dev_monome_set_led(md, x, y, z);
  return 0;

args_error:
  printf("warning: incorrect arguments to grid_set_led() \n"); fflush(stdout);
  return 1;
}

int w_grid_refresh(lua_State *l) {
  struct dev_monome *md;
  if(lua_gettop(l) != 1) { // check num args
    goto args_error;
  }
  if( lua_islightuserdata(l, 1) ) {
    md = lua_touserdata(l, 1);
  } else {
    goto args_error;
  }
  dev_monome_refresh(md);
  return 0;
args_error:
  printf("warning: incorrect arguments to grid_refresh() \n"); fflush(stdout);
  return 1;
}

//-- audio processing controls
int w_load_engine(lua_State *l) {
  if(lua_gettop(l) != 1) {
    goto args_error;
  }

  if( lua_isstring(l, 1) ) {
    o_load_engine( lua_tostring(l, 1) );
    return 0;
  } else {
    goto args_error;
  }

args_error:
  printf("warning: incorrect arguments to load_engine() \n"); fflush(stdout);
  return 1;
}

int w_send_command(lua_State *l) {
  int nargs = lua_gettop(l);
  if(nargs < 1) { goto args_error; }

  char *cmd = NULL;
  char *fmt = NULL;

  if( lua_isnumber(l, 1) ) {
    int idx = (int)lua_tonumber(l, 1) - 1; // 1-base to 0-base
    // FIXME? guess should be wrapped in descriptor access lock...
    // but this will not be called often and a collision seems unlikely here
    cmd = o_get_commands()[idx].name;
    fmt = o_get_commands()[idx].format;
  } else {
    printf("failed type check on first arg \n");
    goto args_error;
  }

  // FIXME: refactor this, should go in oracle!
  lo_message msg = lo_message_new();

  const char *s;
  int d;
  double f;

  for(int i = 2; i <= nargs; i++) {
    switch(fmt[i - 2]) {
    case 's':
      if( lua_isstring(l, i) ) {
        s = lua_tostring(l, i);
        lo_message_add_string(msg, s );
      } else {
        printf("failed string type check \n");
        goto args_error;
      }
      break;
    case 'i':
      if( lua_isnumber(l, i) ) {
        d =  (int)lua_tonumber(l, i);
        lo_message_add_int32( msg, d);
      } else {
        printf("failed int type check \n");
        goto args_error;
      }
      break;
    case 'f':
      if( lua_isnumber(l, i) ) {
        f = lua_tonumber(l, i);
        lo_message_add_double( msg, f );
      } else {
        printf("failed double type check \n");
        goto args_error;
      }
      break;
    default:
      break;
    } /* switch */
  }

  if( (cmd == NULL) || (fmt == NULL) ) {
    printf("error: null format/command string \n");
    return 1;
  } else {
    o_send_command(cmd, msg);
  }
  return 0;

args_error:
  printf("warning: incorrect arguments to send_command() \n"); fflush(stdout);
  return 1;
}

int w_request_engine_report(lua_State *l) {
  (void)l;
  o_request_engine_report();
  return 0;
}

int w_request_command_report(lua_State *l) {
  (void)l;
  o_request_command_report();
  return 0;
}

//--- timer management:

int w_timer_start(lua_State *l) {
  static int idx = 0;
  double seconds;
  int count, stage;
  int nargs = lua_gettop(l);
  if(nargs > 0) {                   // idx
    if( lua_isnumber(l, 1) ) {
      idx = lua_tonumber(l, 1) - 1; // convert from 1-based
    } else {
      goto args_error;
    }
  }
  if(nargs > 1) { // seconds
    if( lua_isnumber(l, 2) ) {
      seconds = lua_tonumber(l, 2);
    } else {
      goto args_error;
    }
  } else {
    seconds = 0.0; // timer will re-use previous value
  }
  if(nargs > 2) {  // count
    if( lua_isnumber(l, 3) ) {
      count = lua_tonumber(l, 3);
    } else {
      goto args_error;
    }
  } else {
    count = -1;                       // default: infinite
  }
  if(nargs > 3) {                     // stage
    if( lua_isnumber(l, 4) ) {
      stage = lua_tonumber(l, 4) - 1; // convert from 1-based
    } else {
      goto args_error;
    }
  } else {
    stage = 0;
  }
  timer_start(idx, seconds, count, stage);
  return 0;
args_error:
  printf("warning: incorrect argument(s) to start_timer(); expected [i(fii)] \n");
  fflush(stdout);
  return 1;
}

int w_timer_stop(lua_State *l) {
  int idx;
  if(lua_gettop(l) != 1) {
    goto args_error;
  }
  if( lua_isnumber(l, 1) ) {
    idx = lua_tonumber(l, 1) - 1;
  } else {
    goto args_error;
  }
  timer_stop(idx);
  return 0;
args_error:
  printf("warning: incorrect arguments to stop_timer(); expected [i] \n");
  fflush(stdout);
  return 1;
}


// request current time since Epoch
int w_get_time(lua_State *l) {
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  // returns two results: seconds, microseconds
  lua_pushinteger(l, (lua_Integer)tv.tv_sec);
  lua_pushinteger(l, (lua_Integer)tv.tv_usec);
  return 2;
}

//---- c -> lua glue

//--- hardware input:

// helper for calling grid handlers
static inline void
w_call_grid_handler(int id, int x, int y, int state) {
  w_push_norns_func("grid", "key");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  lua_pushinteger(lvm, x + 1);  // convert to 1-base
  lua_pushinteger(lvm, y + 1);  // convert to 1-base
  lua_pushinteger(lvm, state);
  l_report( lvm, l_docall(lvm, 4, 0) );
}

void w_handle_monome_add(void *mdev) {
  struct dev_monome *md = (struct dev_monome *)mdev;
  int id = md->dev.id;
  const char *serial = md->dev.serial;
  const char *name =  md->dev.name;
  w_push_norns_func("monome", "add");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  lua_pushstring(lvm, serial);
  lua_pushstring(lvm, name);
  lua_pushlightuserdata(lvm, mdev);
  l_report( lvm, l_docall(lvm, 4, 0) );
}

void w_handle_monome_remove(int id) {
  w_push_norns_func("monome", "remove");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  l_report( lvm, l_docall(lvm, 1, 0) );
}

void w_handle_grid_key(int id, int x, int y, int state) {
  w_call_grid_handler( id, x, y, state > 0);
}

void w_handle_input_add(void *p) {
  struct dev_input *dev = (struct dev_input *)p;
  struct dev_common *base = (struct dev_common *)p;
  int id = base->id;

  w_push_norns_func("input", "add");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  lua_pushstring(lvm, base->serial);
  lua_pushstring(lvm, base->name);

  // push table of event types
  int ntypes = dev->num_types;
  lua_createtable(lvm, ntypes, 0);
  for(int i = 0; i < ntypes; i++) {
    lua_pushinteger(lvm, dev->types[i]);
    lua_rawseti(lvm, -2,  i + 1);
  }

  // table of tables of event codes
  lua_createtable(lvm, ntypes, 0);
  for(int i = 0; i < ntypes; i++) {
    int ncodes = dev->num_codes[i];
    lua_createtable(lvm, ncodes, 0);
    for(int j = 0; j < ncodes; j++) {
      lua_pushinteger(lvm, dev->codes[i][j]);
      lua_rawseti(lvm, -2, j + 1);
    }
    lua_rawseti(lvm, -2, i + 1);
  }
  l_report( lvm, l_docall(lvm, 5, 0) );
}

void w_handle_input_remove(int id) {
  (void)id;
  //... TODO!
}

void w_handle_input_event(int id, uint8_t type, dev_code_t code, int value) {
  w_push_norns_func("input", "event");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  lua_pushinteger(lvm, type);
  lua_pushinteger(lvm, code);
  lua_pushinteger(lvm, value);
  l_report( lvm, l_docall(lvm, 4, 0) );
}

// helper for pushing array of c strings
static inline void
w_push_string_array(const char **arr, const int n) {
  // push a table of strings
  lua_createtable(lvm, n, 0);
  for (int i = 0; i < n; i++) {
    lua_pushstring(lvm, arr[i]);
    lua_rawseti(lvm, -2, i + 1);
  }
  // push count of entries
  lua_pushinteger(lvm, n);
}

// audio engine report handlers
void w_handle_engine_report(const char **arr, const int n) {
  w_push_norns_func("report", "engines");
  w_push_string_array(arr, n);
  l_report( lvm, l_docall(lvm, 2, 0) );
}

void w_handle_command_report(const struct engine_command *arr,
                             const int num) {
  w_push_norns_func("report", "commands");
  // push a table of tables: {{cmd, fmt}, {cmd,fmt}, ...}
  lua_createtable(lvm, num, 0);
  for(int i = 0; i < num; i++) {
    // create subtable on stack
    lua_createtable(lvm, 2, 0);
    // put command string on stack; assign to subtable; pop
    lua_pushstring(lvm, arr[i].name);
    lua_rawseti(lvm, -2, 1);
    lua_pushstring(lvm, arr[i].format);
    // put format string on stack; assign to subtable; pop
    lua_rawseti(lvm, -2, 2);
    // subtable is on stack; assign to master table and pop
    lua_rawseti(lvm, -2, i + 1);
  }
  // second return value is table size
  lua_pushinteger(lvm, num);
  l_report( lvm, l_docall(lvm, 2, 0) );
}

void w_handle_poll_report(const struct engine_poll *arr,
			  const int num) {
  (void)arr;
  (void)num;
  // TODO!
}

// timer handler
void w_handle_timer(const int idx, const int stage) {
  lua_getglobal(lvm, "norns");
  lua_getfield(lvm, -1, "timer");
  lua_remove(lvm, -2);
  lua_pushinteger(lvm, idx + 1);   // convert to 1-based
  lua_pushinteger(lvm, stage + 1); // convert to 1-based
  l_report( lvm, l_docall(lvm, 2, 0) );
}


