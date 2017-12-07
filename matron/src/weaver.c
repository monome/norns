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
#include "device_hid.h"
#include "device_monome.h"
#include "events.h"
#include "lua_eval.h"
#include "metro.h"
#include "screen.h"
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
static int w_grid_all_led(lua_State *l);
static int w_grid_refresh(lua_State *l);
//screen
static int w_screen_aa(lua_State *l);
static int w_screen_level(lua_State *l);
static int w_screen_line_width(lua_State *l);
static int w_screen_move(lua_State *l);
static int w_screen_line(lua_State *l);
static int w_screen_stroke(lua_State *l);
static int w_screen_text(lua_State *l);
static int w_screen_clear(lua_State *l);

// crone
/// engines
static int w_request_engine_report(lua_State *l);
static int w_load_engine(lua_State *l);
/// commands
static int w_request_command_report(lua_State *l);
static int w_send_command(lua_State *l);
static int w_request_poll_report(lua_State *l);
static int w_start_poll(lua_State *l);
static int w_stop_poll(lua_State *l);
static int w_set_poll_time(lua_State *l);
// timing
static int w_metro_start(lua_State *l);
static int w_metro_stop(lua_State *l);
static int w_metro_set_time(lua_State *l);
// get the current system time
static int w_get_time(lua_State *l);

// boilerplate: push a function to the stack, from field in global 'norns'
static inline void
w_push_norns_func(const char *field, const char *func) {
    //    printf("calling norns.%s.%s\n", field, func); fflush(stdout);
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

    ////////////////////////
    // FIXME: document these in lua in some deliberate fashion
    //////////////////

    // low-level monome grid control
    lua_register(lvm, "grid_set_led", &w_grid_set_led);
    lua_register(lvm, "grid_all_led", &w_grid_all_led);
    lua_register(lvm, "grid_refresh", &w_grid_refresh);

    // register screen funcs
    lua_register(lvm, "s_aa", &w_screen_aa);
    lua_register(lvm, "s_level", &w_screen_level);
    lua_register(lvm, "s_line_width", &w_screen_line_width);
    lua_register(lvm, "s_move", &w_screen_move);
    lua_register(lvm, "s_line", &w_screen_line);
    lua_register(lvm, "s_stroke", &w_screen_stroke);
    lua_register(lvm, "s_text", &w_screen_text);
    lua_register(lvm, "s_clear", &w_screen_clear);

    // get list of available crone engines
    lua_register(lvm, "report_engines", &w_request_engine_report);
    // load a named engine
    lua_register(lvm, "load_engine", &w_load_engine);

    // get list of available crone commmands based on current engine
    lua_register(lvm, "report_commands", &w_request_command_report);
    // send an indexed command
    lua_register(lvm, "send_command", &w_send_command);

    // start/stop an indexed metro with callback
    lua_register(lvm, "metro_start", &w_metro_start);
    lua_register(lvm, "metro_stop", &w_metro_stop);
    lua_register(lvm, "metro_set_time", &w_metro_set_time);

    // get the current high-resolution CPU time
    lua_register(lvm, "get_time", &w_get_time);

    // report available polling functions
    lua_register(lvm, "report_polls", &w_request_poll_report);
    // start / stop a poll
    lua_register(lvm, "start_poll", &w_start_poll);
    lua_register(lvm, "stop_poll", &w_stop_poll);
    lua_register(lvm, "set_poll_time", &w_set_poll_time);

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
//
int w_screen_aa(lua_State *l) {
    int x;
    if(lua_gettop(l) != 1) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        x = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    screen_aa(x);
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to s_aa() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

int w_screen_level(lua_State *l) {
    int x;
    if(lua_gettop(l) != 1) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        x = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    screen_level(x);
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to s_level() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

int w_screen_line_width(lua_State *l) {
    long x;
    if(lua_gettop(l) != 1) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        x = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    screen_line_width(x);
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to s_line_width() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

int w_screen_move(lua_State *l) {
    int x, y;
    if(lua_gettop(l) != 2) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        x = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 2) ) {
        y = lua_tonumber(l, 2);
    } else {
        goto args_error;
    }

    screen_move(x,y);
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to s_move() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

int w_screen_line(lua_State *l) {
    int x, y;
    if(lua_gettop(l) != 2) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        x = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 2) ) {
        y = lua_tonumber(l, 2);
    } else {
        goto args_error;
    }

    screen_line(x,y);
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to s_line() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

int w_screen_stroke(lua_State *l) {
    if(lua_gettop(l) != 0) { // check num args
        goto args_error;
    }

    screen_stroke();
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to s_stroke() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

int w_screen_text(lua_State *l) {
    char s[64];

    if(lua_gettop(l) != 1) { // check num args
        goto args_error;
    }

    if( lua_isstring(l,1) ) {
	strcpy(s,lua_tostring(l,1));
    } else {
        goto args_error;
    }

    screen_text(s);
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to s_text() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

int w_screen_clear(lua_State *l) {
    if(lua_gettop(l) != 0) { // check num args
        goto args_error;
    }

    screen_clear();
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to s_clear() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}


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
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to grid_set_led() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

int w_grid_all_led(lua_State *l) {
    struct dev_monome *md;
    int z;
    if(lua_gettop(l) != 2) { // check num args
        goto args_error;
    }
    if( lua_islightuserdata(l, 1) ) {
        md = lua_touserdata(l, 1);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 2) ) {
        z = lua_tonumber(l, 2) - 1; // convert from 1-base
    } else {
        goto args_error;
    }

    dev_monome_all_led(md, z);
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to grid_all_led() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
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
    lua_settop(l, 0);
    return 0;
args_error:
    printf("warning: incorrect arguments to grid_refresh() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

//-- audio processing controls
int w_load_engine(lua_State *l) {
    if(lua_gettop(l) != 1) {
        goto args_error;
    }

    if( lua_isstring(l, 1) ) {
        o_load_engine( lua_tostring(l, 1) );
        lua_settop(l, 0);
        return 0;
    } else {
        goto args_error;
    }

args_error:
    printf("warning: incorrect arguments to load_engine() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

int w_send_command(lua_State *l) {
    int nargs = lua_gettop(l);
    if(nargs < 1) { goto args_error; }

    char *cmd = NULL;
    char *fmt = NULL;

    if( lua_isnumber(l, 1) ) {
        int idx = (int)lua_tonumber(l, 1) - 1; // 1-base to 0-base
        // FIXME: this isn't really safe.
        // to make it safe would mean locks, which is bad.
        // might be better to put name/fmt on stack from lua on every call
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
        lua_settop(l, 0);
        return 0;
    } else {
        o_send_command(cmd, msg);
    }
    lua_settop(l, 0);
    return 0;
args_error:
    printf("warning: incorrect arguments to send_command() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
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

//--- metro management:
int w_metro_start(lua_State *l) {
    static int idx = 0;
    double seconds;
    int count, stage;
    int nargs = lua_gettop(l);

    if(nargs > 0) {                       // idx
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
        seconds = -1.0; // metro will re-use previous value
    }

    if(nargs > 2) {     // count
        if( lua_isnumber(l, 3) ) {
            count = lua_tonumber(l, 3);
        } else {
            goto args_error;
        }
    } else {
        count = -1;                         // default: infinite
    }
    if(nargs > 3) {                         // stage
        if( lua_isnumber(l, 4) ) {
            stage = lua_tonumber(l, 4) - 1; // convert from 1-based
        } else {
            goto args_error;
        }
    } else {
        stage = 0;
    }
    metro_start(idx, seconds, count, stage);
    lua_settop(l, 0);
    return 0;
args_error:
    printf(
        "warning: incorrect argument(s) to start_metro(); expected [i(fii)] \n");
    fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

int w_metro_stop(lua_State *l) {
    int idx; 
    int nargs = lua_gettop(l);
    if( nargs != 1) {
        goto args_error;
    }
    if( lua_isnumber(l, 1) ) {
        idx = lua_tonumber(l, 1) - 1;
    } else {
        goto args_error;
    }
    metro_stop(idx);
    lua_settop(l, 0);
    return 0;
args_error:
    printf("warning: incorrect arguments to stop_metro(); expected [i] \n");
    fflush(stdout);
    lua_settop(l, 0);
    return 1;
}

int w_metro_set_time(lua_State *l) {
  int idx;
  float sec;
  int nargs = lua_gettop(l);
  if(nargs != 2) {
    goto args_error;
  }
  if( lua_isnumber(l, 1) ) {
    idx = lua_tonumber(l, 1) - 1;
  } else {
    goto args_error;
  }
  if( lua_isnumber(l, 2) ) {
    sec = lua_tonumber(l, 2);
  } else {
    goto args_error;
  }
  metro_set_time(idx, sec);
  lua_settop(l, 0);
  return 0;
 args_error:
  printf("warning: incorrect arguments to metro_set_time(); expected [if] \n");
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

void w_handle_hid_add(void *p) {
    struct dev_hid *dev = (struct dev_hid *)p;
    struct dev_common *base = (struct dev_common *)p;
    int id = base->id;

    w_push_norns_func("hid", "add");
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

void w_handle_hid_remove(int id) {
    w_push_norns_func("hid", "remove");
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    l_report( lvm, l_docall(lvm, 1, 0) );
}

void w_handle_hid_event(int id, uint8_t type, dev_code_t code, int value) {
    w_push_norns_func("hid", "event");
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
        // put command string on stack; assign to subtable, pop
        lua_pushstring(lvm, arr[i].name);
        lua_rawseti(lvm, -2, 1);
        // put format string on stack; assign to subtable, pop
        lua_pushstring(lvm, arr[i].format);
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

    // printf("w_handle_poll_report\n"); fflush(stdout);

    w_push_norns_func("report", "polls");
    lua_createtable(lvm, num, 0);

    for(int i = 0; i < num; ++i) {
        // create subtable on stack
        lua_createtable(lvm, 2, 0);
        // put poll index on stack; assign to subtable, pop
        lua_pushinteger(lvm, i + 1); // convert to 1-base
        lua_rawseti(lvm, -2, 1);
        // put poll name on stack; assign to subtable, pop
        lua_pushstring(lvm, arr[i].name);
        lua_rawseti(lvm, -2, 2);
        if(arr[i].type == POLL_TYPE_VALUE) {
            lua_pushstring(lvm, "value");
        } else {
            lua_pushstring(lvm, "data");
        }
        // put type string on stack; assign to subtable, pop
        lua_rawseti(lvm, -2, 3);
        // subtable is on stack; assign to master table and pop
        lua_rawseti(lvm, -2, i + 1); // convert to 1-base
    }
    lua_pushinteger(lvm, num);
    l_report( lvm, l_docall(lvm, 2, 0) );
}

// metro handler
void w_handle_metro(const int idx, const int stage) {
    lua_getglobal(lvm, "norns");
    lua_getfield(lvm, -1, "metro");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, idx + 1);   // convert to 1-based
    lua_pushinteger(lvm, stage + 1); // convert to 1-based
    l_report( lvm, l_docall(lvm, 2, 0) );
}

// gpio handler
void w_handle_key(const int n, const int val) {
    lua_getglobal(lvm, "norns");
    lua_getfield(lvm, -1, "key");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, n);
    lua_pushinteger(lvm, val);
    l_report( lvm, l_docall(lvm, 2, 0) );
}

// gpio handler
void w_handle_enc(const int n, const int delta) {
    lua_getglobal(lvm, "norns");
    lua_getfield(lvm, -1, "enc");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, n);
    lua_pushinteger(lvm, delta);
    l_report( lvm, l_docall(lvm, 2, 0) );
}

// system/battery
void w_handle_battery(const int percent) {
    lua_getglobal(lvm, "norns");
    lua_getfield(lvm, -1, "battery");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, percent);
    l_report( lvm, l_docall(lvm, 1, 0) );
}

// system/power
void w_handle_power(const int present) {
    lua_getglobal(lvm, "norns");
    lua_getfield(lvm, -1, "power");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, present);
    l_report( lvm, l_docall(lvm, 1, 0) );
}


void w_handle_poll_value(int idx, float val) {
    // printf("w_handle_poll_value: %d, %f \n", idx, val); fflush(stdout);
    lua_getglobal(lvm, "norns");
    lua_getfield(lvm, -1, "poll");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, idx + 1); // convert to 1-base
    lua_pushnumber(lvm, val);
    l_report( lvm, l_docall(lvm, 2, 0) );
}

void w_handle_poll_data(int idx, int size, uint8_t *data) {
    lua_getglobal(lvm, "norns");
    lua_getfield(lvm, -1, "poll");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, idx + 1); // convert index to 1-based
    lua_createtable(lvm, size, 0);
    // FIXME: would like a better way of passing a byte array to lua
    for(int i = 0; i < size; ++i) {
        lua_pushinteger(lvm, data[i]);
        lua_rawseti(lvm, -2, 1);
    }
    lua_pushinteger(lvm, size);
    l_report( lvm, l_docall(lvm, 2, 0) );
}

int w_request_poll_report(lua_State *l) {
    (void)l;
    o_request_poll_report();
    return 0;
}

// helper: set poll given by lua to given state
static int poll_set_state(lua_State *l, bool val) {
    int nargs = lua_gettop(l);
    if(nargs != 1) {
        printf("poll_state_state(); wrong argument count\n"); fflush(stdout);
        return 1;
    }
    if( lua_isinteger(l, 1) ) {
        int idx = lua_tointeger(l, 1) - 1; // convert from 1-based
        o_set_poll_state(idx, val);
        lua_settop(l, 0);
        return 0;
    } else {
        printf("poll_state_state(); wrong argument type\n"); fflush(stdout);
        lua_settop(l, 0);
        return 0;
    }
}

int w_start_poll(lua_State *l) {
    return poll_set_state(l, true);
}

int w_stop_poll(lua_State *l) {
    return poll_set_state(l, false);
}

int w_set_poll_time(lua_State *l) {
    (void)l;
    int nargs = lua_gettop(l);
    if(nargs == 2) {
        if( lua_isinteger(l, 1) ) {
            int idx = lua_tointeger(l, 1) - 1; // convert from 1-based
            if( lua_isnumber(l, 2) ) {
                float val = lua_tonumber(l, 2);
                o_set_poll_time(idx, val);
                lua_settop(l, 0);
                return 0;
            }
        }
    }
    printf("wrong arguments for w_set_poll_time(); ");
    printf("expects idx(int), dt(float) \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}
