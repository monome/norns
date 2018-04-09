/***
 * weaver
 * c->lua and lua->c interface
 * @module system
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
#include "device_midi.h"
#include "events.h"
#include "lua_eval.h"
#include "metro.h"
#include "screen.h"
#include "i2c.h" 
#include "osc.h" 
#include "oracle.h"
#include "weaver.h"

//------
//---- global lua state!
lua_State *lvm;

void w_run_code(const char *code) {
    l_dostring(lvm, code, "w_run_code");
    fflush(stdout);
}

void w_handle_exec_code_line(char *line) {
    l_handle_line(lvm, line);
}

//-------------
//--- declare lua->c glue

// NB these static functions are prefixed  with '_'
// to avoid shadowing similar-named extern functions in other moduels (like
// screen)
// and also to distinguish from extern 'w_' functions.

// grid
static int _grid_set_led(lua_State *l);
static int _grid_all_led(lua_State *l);
static int _grid_refresh(lua_State *l);
//screen
static int _screen_update(lua_State *l);
static int _screen_font_face(lua_State *l);
static int _screen_font_size(lua_State *l);
static int _screen_aa(lua_State *l);
static int _screen_level(lua_State *l);
static int _screen_line_width(lua_State *l);
static int _screen_move(lua_State *l);
static int _screen_line(lua_State *l);
static int _screen_move_rel(lua_State *l);
static int _screen_line_rel(lua_State *l);
static int _screen_curve(lua_State *l);
static int _screen_curve_rel(lua_State *l);
static int _screen_arc(lua_State *l);
static int _screen_rect(lua_State *l);
static int _screen_stroke(lua_State *l);
static int _screen_fill(lua_State *l);
static int _screen_text(lua_State *l);
static int _screen_clear(lua_State *l);
static int _screen_close(lua_State *l);
static int _screen_extents(lua_State *l);
//i2c
static int _gain_hp(lua_State *l);
static int _gain_in(lua_State *l);
//osc
static int _osc_send(lua_State *l);
static int _osc_remote_addr(lua_State *l);

// crone
/// engines
static int _request_engine_report(lua_State *l);
static int _load_engine(lua_State *l);
/// commands
static int _send_command(lua_State *l);
static int _set_parameter_value(lua_State *l);
static int _start_poll(lua_State *l);
static int _stop_poll(lua_State *l);
static int _set_poll_time(lua_State *l);
// timing
static int _metro_start(lua_State *l);
static int _metro_stop(lua_State *l);
static int _metro_set_time(lua_State *l);
// get the current system time
static int _get_time(lua_State *l);

// audio context control
static int _set_audio_input_level(lua_State *l);
static int _set_audio_output_level(lua_State *l);
static int _set_audio_monitor_level(lua_State *l);
static int _set_audio_monitor_mono(lua_State *l);
static int _set_audio_monitor_stereo(lua_State *l);
static int _set_audio_monitor_on(lua_State *l);
static int _set_audio_monitor_off(lua_State *l);
static int _set_audio_pitch_on(lua_State *l);
static int _set_audio_pitch_off(lua_State *l);

// restart audio completely (recompile sclang)
static int _restart_audio(lua_State *l);

// boilerplate: push a function to the stack, from field in global 'norns'
static inline void
_push_norns_func(const char *field, const char *func) {
    // fprintf(stderr, "calling norns.%s.%s\n", field, func);
    lua_getglobal(lvm, "norns");
    lua_getfield(lvm, -1, field);
    lua_remove(lvm, -2);
    lua_getfield(lvm, -1, func);
    lua_remove(lvm, -2);
}

////////////////////////////////
//// extern function definitions

void w_init(void) {
    fprintf(stderr, "starting lua vm\n");
    lvm = luaL_newstate();
    luaL_openlibs(lvm);
    lua_pcall(lvm, 0, 0, 0);

    ////////////////////////
    // FIXME: document these in lua in some deliberate fashion
    //////////////////

    // low-level monome grid control
    lua_register(lvm, "grid_set_led", &_grid_set_led);
    lua_register(lvm, "grid_all_led", &_grid_all_led);
    lua_register(lvm, "grid_refresh", &_grid_refresh);

    // register screen funcs
    lua_register(lvm, "s_update", &_screen_update);
    lua_register(lvm, "s_font_face", &_screen_font_face);
    lua_register(lvm, "s_font_size", &_screen_font_size);
    lua_register(lvm, "s_aa", &_screen_aa);
    lua_register(lvm, "s_level", &_screen_level);
    lua_register(lvm, "s_line_width", &_screen_line_width);
    lua_register(lvm, "s_move", &_screen_move);
    lua_register(lvm, "s_line", &_screen_line);
    lua_register(lvm, "s_move_rel", &_screen_move_rel);
    lua_register(lvm, "s_line_rel", &_screen_line_rel);
    lua_register(lvm, "s_curve", &_screen_curve);
    lua_register(lvm, "s_curve_rel", &_screen_curve_rel);
    lua_register(lvm, "s_arc", &_screen_arc);
    lua_register(lvm, "s_rect", &_screen_rect);
    lua_register(lvm, "s_stroke", &_screen_stroke);
    lua_register(lvm, "s_fill", &_screen_fill);
    lua_register(lvm, "s_text", &_screen_text);
    lua_register(lvm, "s_clear", &_screen_clear);
    lua_register(lvm, "s_close", &_screen_close);
    lua_register(lvm, "s_extents", &_screen_extents);

    // analog output control
    lua_register(lvm, "gain_hp", &_gain_hp);
    lua_register(lvm, "gain_in", &_gain_in);

    // osc
    lua_register(lvm, "osc_send", &_osc_send);
    lua_register(lvm, "osc_remote_addr", &_osc_remote_addr);

    // get list of available crone engines
    lua_register(lvm, "report_engines", &_request_engine_report);
    // load a named engine
    lua_register(lvm, "load_engine", &_load_engine);

    // send an indexed command
    lua_register(lvm, "send_command", &_send_command);

    // set a parameter value
    lua_register(lvm, "set_parameter_value", &_set_parameter_value);

    // start/stop an indexed metro with callback
    lua_register(lvm, "metro_start", &_metro_start);
    lua_register(lvm, "metro_stop", &_metro_stop);
    lua_register(lvm, "metro_set_time", &_metro_set_time);

    // get the current high-resolution CPU time
    lua_register(lvm, "get_time", &_get_time);

    // start / stop a poll
    lua_register(lvm, "start_poll", &_start_poll);
    lua_register(lvm, "stop_poll", &_stop_poll);
    lua_register(lvm, "set_poll_time", &_set_poll_time);

    // audio context controls
    lua_register(lvm, "audio_input_level", &_set_audio_input_level);
    lua_register(lvm, "audio_output_level", &_set_audio_output_level);
    lua_register(lvm, "audio_monitor_level", &_set_audio_monitor_level);
    lua_register(lvm, "audio_monitor_mono", &_set_audio_monitor_mono);
    lua_register(lvm, "audio_monitor_stereo", &_set_audio_monitor_stereo);
    lua_register(lvm, "audio_monitor_on", &_set_audio_monitor_on);
    lua_register(lvm, "audio_monitor_off", &_set_audio_monitor_off);
    lua_register(lvm, "audio_pitch_on", &_set_audio_pitch_on);
    lua_register(lvm, "audio_pitch_off", &_set_audio_pitch_off);

    // completely restart the audio process (recompile sclang)
    lua_register(lvm, "restart_audio", &_restart_audio);

    // run system init code
    char *config = getenv("NORNS_CONFIG");
    char *home = getenv("HOME");
    char cmd[256];

    if(config == NULL) {
        snprintf(cmd, 256, "dofile('%s/norns/lua/config.lua')\n", home);
    } else {
        snprintf(cmd, 256, "dofile('%s')\n", config);
    }
    fprintf(stderr, "running lua config file: %s", cmd);
    w_run_code(cmd);
    w_run_code("require('norns')");
}

// run startup code
// audio backend should be running
void w_startup(void) {
    lua_getglobal(lvm, "startup");
    l_report( lvm, l_docall(lvm, 0, 0) );
}

void w_deinit(void) {
    // FIXME: lua is leaking memory. doesn't really matter
}

//----------------------------------
//---- static definitions

/***
 * screen: update (flip buffer)
 * @function s_update
 */
int _screen_update(lua_State *l) {
    if(lua_gettop(l) != 0) { // check num args
        goto args_error;
    }

    screen_update();
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_update() \n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: set font face
 * @function s_font_face
 */
int _screen_font_face(lua_State *l) {
    int x;
    if(lua_gettop(l) != 1) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        x = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    screen_font_face(x);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_font_face() \n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: set font size
 * @function s_font_size
 */
int _screen_font_size(lua_State *l) {
    long x;
    if(lua_gettop(l) != 1) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        x = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    screen_font_size(x);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_font_size() \n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: change antialias mode for drawing
 * @function s_aa
 * @tparam integer state, 0=off, 1=on
 */
int _screen_aa(lua_State *l) {
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
    fprintf(stderr, "warning: incorrect arguments to s_aa() \n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: change level (color)
 * @function s_level
 * @tparam integer level, 0 (black) to 15 (white)
 */
int _screen_level(lua_State *l) {
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
    fprintf(stderr, "warning: incorrect arguments to s_level() \n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: set line width
 * @function s_line_width
 * @tparam integer width line width
 */
int _screen_line_width(lua_State *l) {
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
    fprintf(stderr, "warning: incorrect arguments to s_line_width() \n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: move position
 * @function s_move
 * @param x
 * @param y
 */
int _screen_move(lua_State *l) {
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
    fprintf(stderr, "warning: incorrect arguments to s_move() \n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: draw line to position
 * @function s_line
 * @param x
 * @param y
 */
int _screen_line(lua_State *l) {
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
    fprintf(stderr, "warning: incorrect arguments to s_line()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: move position rel
 * @function s_move_rel
 * @param x
 * @param y
 */
int _screen_move_rel(lua_State *l) {
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

    screen_move_rel(x,y);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_move_rel()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: draw line to position rel
 * @function s_line_rel
 * @param x
 * @param y
 */
int _screen_line_rel(lua_State *l) {
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

    screen_line_rel(x,y);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_line_rel()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: draw curve
 * @function s_curve
 * @param x
 * @param y
 */
int _screen_curve(lua_State *l) {
    double x1,y1,x2,y2,x3,y3;
    if(lua_gettop(l) != 6) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        x1 = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 2) ) {
        y1 = lua_tonumber(l, 2);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 3) ) {
        x2 = lua_tonumber(l, 3);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 4) ) {
        y2 = lua_tonumber(l, 4);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 5) ) {
        x3 = lua_tonumber(l, 5);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 6) ) {
        y3 = lua_tonumber(l, 6);
    } else {
        goto args_error;
    }

    screen_curve(x1,y1,x2,y2,x3,y3);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_curve()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: draw curve rel
 * @function s_curve_rel
 * @param x
 * @param y
 */
int _screen_curve_rel(lua_State *l) {
    double x1,y1,x2,y2,x3,y3;
    if(lua_gettop(l) != 6) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        x1 = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 2) ) {
        y1 = lua_tonumber(l, 2);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 3) ) {
        x2 = lua_tonumber(l, 3);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 4) ) {
        y2 = lua_tonumber(l, 4);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 5) ) {
        x3 = lua_tonumber(l, 5);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 6) ) {
        y3 = lua_tonumber(l, 6);
    } else {
        goto args_error;
    }

    screen_curve_rel(x1,y1,x2,y2,x3,y3);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_curve_rel()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: draw arc
 * @function s_arc
 * @param x
 * @param y
 */
int _screen_arc(lua_State *l) {
    double x, y, r, a1, a2;
    if(lua_gettop(l) != 5) { // check num args
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

    if( lua_isnumber(l, 3) ) {
        r = lua_tonumber(l, 3);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 4) ) {
        a1 = lua_tonumber(l, 4);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 5) ) {
        a2 = lua_tonumber(l, 5);
    } else {
        goto args_error;
    }

    screen_arc(x,y,r,a1,a2);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_arc()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: draw rect
 * @function s_rect
 * @param x
 * @param y
 */
int _screen_rect(lua_State *l) {
    double x,y,w,h;
    if(lua_gettop(l) != 4) { // check num args
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

    if( lua_isnumber(l, 3) ) {
        w = lua_tonumber(l, 3);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 4) ) {
        h = lua_tonumber(l, 4);
    } else {
        goto args_error;
    }

    screen_rect(x,y,w,h);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_rect()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: stroke, or apply width/color to line(s)
 * @function s_stroke
 */
int _screen_stroke(lua_State *l) {
    if(lua_gettop(l) != 0) { // check num args
        goto args_error;
    }

    screen_stroke();
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_stroke()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: fill path
 * @function s_fill
 */
int _screen_fill(lua_State *l) {
    if(lua_gettop(l) != 0) { // check num args
        goto args_error;
    }

    screen_fill();
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_fill()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: text
 * @function s_text
 * @tparam string text test to print
 */
int _screen_text(lua_State *l) {
    char s[64];

    if(lua_gettop(l) != 1) { // check num args
        goto args_error;
    }

    if( lua_isstring(l,1) ) {
        strcpy( s,lua_tostring(l,1) );
    } else {
        goto args_error;
    }

    screen_text(s);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_text()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: clear to black
 * @function s_clear
 */
int _screen_clear(lua_State *l) {
    if(lua_gettop(l) != 0) { // check num args
        goto args_error;
    }

    screen_clear();
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_clear()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: close path
 * @function s_close
 */
int _screen_close(lua_State *l) {
    if(lua_gettop(l) != 0) { // check num args
        goto args_error;
    }

    screen_close_path();
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_close()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: extents
 * @function s_extents
 * @tparam gets x/y displacement of a string
 */
int _screen_extents(lua_State *l) {
    char s[64];
    double *xy;

    if(lua_gettop(l) != 1) { // check num args
        goto args_error;
    }

    if( lua_isstring(l,1) ) {
        strcpy( s,lua_tostring(l,1) );
    } else {
        goto args_error;
    }

    xy = screen_extents(s);
    //lua_settop(l, 0);
    lua_pushinteger(l, xy[0]);
    lua_pushinteger(l, xy[1]);
    return 2;

args_error:
    fprintf(stderr, "warning: incorrect arguments to s_extents()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * headphone: set level
 * @function gain_hp
 * @tparam integer level level (0-63)
 */
int _gain_hp(lua_State *l) {
    int level;
    if(lua_gettop(l) != 1) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        level = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    i2c_hp(level);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to gain_hp()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * input: set gain, per channel
 * @function gain_in
 * @tparam integer level level (0-63)
 * @tparam integer ch channel (0=L,1=R)
 */
int _gain_in(lua_State *l) {
    int level, ch;
    if(lua_gettop(l) != 2) { // check num args
        goto args_error;
    }

    if( lua_isnumber(l, 1) ) {
        level = lua_tonumber(l, 1);
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 2) ) {
        ch = lua_tonumber(l, 2);
    } else {
        goto args_error;
    }

    i2c_gain(level,ch);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to gain_in()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * osc: send
 * @function osc_send
 */
int _osc_send(lua_State *l) {
    int nargs = lua_gettop(l);
    if(nargs < 1) { goto args_error; }

    char path[64];

    if( lua_type(l, 1) == LUA_TSTRING ) {
        strcpy( path,lua_tostring(l,1) );
    } else {
        goto args_error;
    }

    lo_message msg = lo_message_new();
    const char *s;
    double f;

    for(int i = 2; i <= nargs; i++) {
        if( lua_type(l, i) == LUA_TNUMBER) {
            f = lua_tonumber(l, i);
            lo_message_add_double( msg, f );
        } else if( lua_type(l, i) == LUA_TSTRING ) {
            s = lua_tostring(l, i);
            lo_message_add_string(msg, s);
        } else {
            //goto args_error;
            break;
        }
    }

    osc_send(path,msg);
    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to osc_send() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}

/***
 * osc: set remote address
 * @function osc_remote_addr
 */
int _osc_remote_addr(lua_State *l) {
    int nargs = lua_gettop(l);
    if(nargs != 2) { goto args_error; }

    char ip[24];
    char port[16];

    if( lua_type(l, 1) == LUA_TSTRING ) {
        strcpy( ip,lua_tostring(l,1) );
    } else {
        goto args_error;
    }

    if( lua_isnumber(l, 2)) {
        strcpy( port,lua_tostring(l,2) );
    } else {
        goto args_error;
    }

    osc_remote_addr(ip,port);
    //printf("ip: %s @ %s",ip,port); fflush(stdout);

    lua_settop(l, 0);
    return 0;

args_error:
    printf("warning: incorrect arguments to osc_remote_addr() \n"); fflush(stdout);
    lua_settop(l, 0);
    return 0;
}


/***
 * grid: set led
 * @function grid_set_led
 * @param dev grid device
 * @param x x
 * @param y y
 * @param z level (0-15)
 */
int _grid_set_led(lua_State *l) {
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
    fprintf(stderr, "warning: incorrect arguments to grid_set_led()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * grid: set all LEDs
 * @function grid_all_led
 * @param dev grid device
 * @param z level (0-15)
 */
int _grid_all_led(lua_State *l) {
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
        z = lua_tonumber(l, 2); // don't convert value!
    } else {
        goto args_error;
    }

    dev_monome_all_led(md, z);
    lua_settop(l, 0);
    return 0;

args_error:
    fprintf(stderr, "warning: incorrect arguments to grid_all_led()\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * grid: refresh
 * @function grid_refresh
 * @param dev grid device
 */
int _grid_refresh(lua_State *l) {
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
    fprintf(stderr, "warning: incorrect arguments to grid_refresh()\n");
    lua_settop(l, 0);
    return 0;
}

//-- audio processing controls
int _load_engine(lua_State *l) {
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
    fprintf(stderr, "warning: incorrect arguments to load_engine()\n");
    lua_settop(l, 0);
    return 0;
}

int _send_command(lua_State *l) {
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
        fprintf(stderr, "failed type check on first arg\n");
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
                lo_message_add_string(msg, s);
            } else {
                fprintf(stderr, "failed string type check\n");
                goto args_error;
            }
            break;
        case 'i':
            if( lua_isnumber(l, i) ) {
                d =  (int)lua_tonumber(l, i);
                lo_message_add_int32(msg, d);
            } else {
                fprintf(stderr, "failed int type check\n");
                goto args_error;
            }
            break;
        case 'f':
            if( lua_isnumber(l, i) ) {
                f = lua_tonumber(l, i);
                lo_message_add_double(msg, f);
            } else {
                fprintf(stderr, "failed double type check\n");
                goto args_error;
            }
            break;
        default:
            break;
        } /* switch */
    }
    if( (cmd == NULL) || (fmt == NULL) ) {
        fprintf(stderr, "error: null format/command string\n");
        lua_settop(l, 0);
        return 0;
    } else {
        o_send_command(cmd, msg);
    }
    lua_settop(l, 0);
    return 0;
args_error:
    fprintf(stderr, "warning: incorrect arguments to send_command()\n");
    lua_settop(l, 0);
    return 0;
}

int _set_parameter_value(lua_State *l) {
    int nargs = lua_gettop(l);
    if(nargs == 2) {
        if( lua_isinteger(l, 1) ) {
            int bus = lua_tointeger(l, 1);
            if( lua_isnumber(l, 2) ) {
                float val = lua_tonumber(l, 2);
                o_set_parameter_value(bus, val);
                lua_settop(l, 0);
                return 0;
            }
        }
    }
    fprintf(stderr, "wrong arguments for _set_parameter_value(); ");
    fprintf(stderr, "expects bus(int), value(float)\n");
    lua_settop(l, 0);
    return 1;
}

int _request_engine_report(lua_State *l) {
    (void)l;
    o_request_engine_report();
    return 0;
}


/***
 * metro: start
 * @function metro_start
 */
int _metro_start(lua_State *l) {
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
    fprintf(stderr,
        "warning: incorrect argument(s) to start_metro(); expected [i(fii)]\n");
    lua_settop(l, 0);
    return 0;
}

/***
 * metro: stop
 * @function metro_stop
 */
int _metro_stop(lua_State *l) {
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
    fprintf(stderr, "warning: incorrect arguments to stop_metro(); expected [i]\n");
    lua_settop(l, 0);
    return 1;
}

/***
 * metro: set time
 * @function metro_set_time
 */
int _metro_set_time(lua_State *l) {
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
    fprintf(stderr,
        "warning: incorrect arguments to metro_set_time(); expected [if]\n");
    return 1;
}

/***
 * request current time since Epoch
 * @function get_time
 */
int _get_time(lua_State *l) {
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
_call_grid_handler(int id, int x, int y, int state) {
    _push_norns_func("grid", "key");
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
    _push_norns_func("monome", "add");
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    lua_pushstring(lvm, serial);
    lua_pushstring(lvm, name);
    lua_pushlightuserdata(lvm, mdev);
    l_report( lvm, l_docall(lvm, 4, 0) );
}

void w_handle_monome_remove(int id) {
    _push_norns_func("monome", "remove");
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    l_report( lvm, l_docall(lvm, 1, 0) );
}

void w_handle_grid_key(int id, int x, int y, int state) {
    _call_grid_handler(id, x, y, state > 0);
}

void w_handle_hid_add(void *p) {
    struct dev_hid *dev = (struct dev_hid *)p;
    struct dev_common *base = (struct dev_common *)p;
    int id = base->id;

    _push_norns_func("hid", "add");
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
    _push_norns_func("hid", "remove");
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    l_report( lvm, l_docall(lvm, 1, 0) );
}

void w_handle_hid_event(int id, uint8_t type, dev_code_t code, int value) {
    _push_norns_func("hid", "event");
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    lua_pushinteger(lvm, type);
    lua_pushinteger(lvm, code);
    lua_pushinteger(lvm, value);
    l_report( lvm, l_docall(lvm, 4, 0) );
}

void w_handle_midi_add(void *p) {
    //struct dev_midi *dev = (struct dev_midi *)p;
    struct dev_common *base = (struct dev_common *)p;
    int id = base->id;

    _push_norns_func("midi", "add");
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    l_report(lvm, l_docall(lvm, 1, 0));
}

void w_handle_midi_remove(int id) {
    _push_norns_func("midi", "remove");
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    l_report(lvm, l_docall(lvm, 1, 0));
}

void w_handle_midi_event(int id, uint8_t *data) {
    _push_norns_func("midi", "event");
    // TODO: params
    lua_pushinteger(lvm, id);
    lua_pushinteger(lvm, data[0]);
    lua_pushinteger(lvm, data[1]);
    lua_pushinteger(lvm, data[2]);
    l_report( lvm, l_docall(lvm, 4, 0) );
}

// helper for pushing array of c strings
static inline void
_push_string_array(const char **arr, const int n) {
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
    _push_norns_func("report", "engines");
    _push_string_array(arr, n);
    l_report( lvm, l_docall(lvm, 2, 0) );
}

// helper: push table of commands
// each entry is a subtatble: {name, format}
static void _push_commands() {
  o_lock_descriptors();
    const struct engine_command *p = o_get_commands();
    const int n = o_get_num_commands();
    lua_createtable(lvm, n, 0);
        for(int i = 0; i < n; i++) {
        // create subtable on stack
        lua_createtable(lvm, 2, 0);
        // put command string on stack; assign to subtable, pop
        lua_pushstring(lvm, p[i].name);
        lua_rawseti(lvm, -2, 1);
        // put format string on stack; assign to subtable, pop
        lua_pushstring(lvm, p[i].format);
        lua_rawseti(lvm, -2, 2);
        // subtable is on stack; assign to master table and pop
        lua_rawseti(lvm, -2, i + 1);
    }
    o_unlock_descriptors();    
    lua_pushinteger(lvm, n);
}

// helper: push table of parameters
// each entry is a subtatble: {name, format}
static void _push_params() {
  o_lock_descriptors();
    const struct engine_param *p = o_get_params();
    const int n = o_get_num_params();
    lua_createtable(lvm, n, 0);
        for(int i = 0; i < n; i++) {
        // create subtable on stack
        lua_createtable(lvm, 2, 0);
        // put name string on stack; assign to subtable, pop
        lua_pushstring(lvm, p[i].name);
        lua_rawseti(lvm, -2, 1);
        // put bus index on stack; assign to subtable, pop
        lua_pushinteger(lvm, p[i].busIdx); // *not* converting to 1-base here
        lua_rawseti(lvm, -2, 2);
        // put minval on stack; assign to subtable, pop
        lua_pushnumber(lvm, p[i].minval);
        lua_rawseti(lvm, -2, 3);
        // put maxval on stack; assign to subtable, pop
        lua_pushnumber(lvm, p[i].maxval);
        lua_rawseti(lvm, -2, 4);
        // put warp string on stack; assign to subtable, pop
        lua_pushstring(lvm, p[i].warp);
        lua_rawseti(lvm, -2, 5);
        // put step on stack; assign to subtable, pop
        lua_pushnumber(lvm, p[i].step);
        lua_rawseti(lvm, -2, 6);
        // put default value on stack; assign to subtable, pop
        lua_pushnumber(lvm, p[i].default_value);
        lua_rawseti(lvm, -2, 7);
        // put units string on stack; assign to subtable, pop
        lua_pushstring(lvm, p[i].units);
        lua_rawseti(lvm, -2, 8);

        // subtable is on stack; assign to master table and pop
        lua_rawseti(lvm, -2, i + 1);
    }
    o_unlock_descriptors();    
    lua_pushinteger(lvm, n);
}

// helper: push table of polls
// each entry is a subtable: { name, type }
// FIXME: this is silly, just use full format specification as for commands
static void _push_polls() {
  o_lock_descriptors();
    const struct engine_poll *p = o_get_polls();
    const int n = o_get_num_polls();
    lua_createtable(lvm, n, 0);
    for(int i = 0; i < n; ++i) {
        // create subtable on stack
        lua_createtable(lvm, 2, 0);
        // put poll index on stack; assign to subtable, pop
        lua_pushinteger(lvm, i + 1); // convert to 1-base
        lua_rawseti(lvm, -2, 1);
        // put poll name on stack; assign to subtable, pop
        lua_pushstring(lvm, p[i].name);
        lua_rawseti(lvm, -2, 2);
	/// FIXME: just use a format string.... 
        if(p[i].type == POLL_TYPE_VALUE) {
            lua_pushstring(lvm, "value");
        } else {
            lua_pushstring(lvm, "data");
        }
        // put type string on stack; assign to subtable, pop
        lua_rawseti(lvm, -2, 3);
        // subtable is on stack; assign to master table and pop
        lua_rawseti(lvm, -2, i + 1); // convert to 1-base
    }    
    o_unlock_descriptors();
    lua_pushinteger(lvm, n);
}

void w_handle_engine_loaded() {
  fprintf(stderr, "w_handle_engine_loaded()\n");

  _push_norns_func("report", "commands");
  _push_commands();
  l_report(lvm, l_docall(lvm, 2, 0));
  
  _push_norns_func("report", "params");
  _push_params();
  l_report(lvm, l_docall(lvm, 2, 0));
  
  _push_norns_func("report", "polls");
  _push_polls();
  l_report(lvm, l_docall(lvm, 2, 0));
  
  _push_norns_func("report", "did_engine_load");
  l_report(lvm, l_docall(lvm, 0, 0));
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
void w_handle_battery(const int percent, const int current) {
    lua_getglobal(lvm, "norns");
    lua_getfield(lvm, -1, "battery");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, percent);
    lua_pushinteger(lvm, current);
    l_report( lvm, l_docall(lvm, 2, 0) );
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
    // fprintf(stderr, "_handle_poll_value: %d, %f\n", idx, val);
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
    // FIXME: would like a better way of passing a byte array to lua!
    for(int i = 0; i < size; ++i) {
        lua_pushinteger(lvm, data[i]);
        lua_rawseti(lvm, -2, 1);
    }
    lua_pushinteger(lvm, size);
    l_report( lvm, l_docall(lvm, 2, 0) );
}

/* void w_handle_poll_wave(int idx, uint8_t *data) { */
/*   // TODO */
/* } */

// argument is an array of 4 bytes
void w_handle_poll_io_levels(uint8_t *levels) {
    lua_getglobal(lvm, "norns");
    lua_getfield(lvm, -1, "vu");
    lua_remove(lvm, -2);
    for(int i = 0; i < 4; ++i) {
        lua_pushinteger(lvm, levels[i]);
    }
    l_report( lvm, l_docall(lvm, 4, 0) );
}


// helper: set poll given by lua to given state
static int poll_set_state(lua_State *l, bool val) {
    int nargs = lua_gettop(l);
    if(nargs != 1) {
        fprintf(stderr, "poll_state_state(); wrong argument count\n");
        return 1;
    }
    if( lua_isinteger(l, 1) ) {
        int idx = lua_tointeger(l, 1) - 1; // convert from 1-based
        o_set_poll_state(idx, val);
        lua_settop(l, 0);
        return 0;
    } else {
        fprintf(stderr, "poll_state_state(); wrong argument type\n");
        lua_settop(l, 0);
        return 0;
    }
}

int _start_poll(lua_State *l) {
    return poll_set_state(l, true);
}

int _stop_poll(lua_State *l) {
    return poll_set_state(l, false);
}

int _set_poll_time(lua_State *l) {
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
    fprintf(stderr, "wrong arguments for w_set_poll_time(); ");
    fprintf(stderr, "expects idx(int), dt(float)\n");
    lua_settop(l, 0);
    return 1;
}

// audio context control
int _set_audio_input_level(lua_State *l) {
    int nargs = lua_gettop(l);
    if(nargs == 2) {
        if( lua_isinteger(l, 1) ) {
            int idx = lua_tointeger(l, 1) - 1; // convert from 1-based
            if( lua_isnumber(l, 2) ) {
                float val = lua_tonumber(l, 2);
                o_set_audio_input_level(idx, val);
                lua_settop(l, 0);
                return 0;
            }
        }
    }
    fprintf(stderr, "wrong arguments for _set_audio_input_level; ");
    fprintf(stderr, "expects idx(int), level(float)\n");
    lua_settop(l, 0);
    return 1;
}

int _set_audio_output_level(lua_State *l) {
    int nargs = lua_gettop(l);
    if(nargs == 1) {
        if( lua_isnumber(l, 1) ) {
            float val = lua_tonumber(l, 1);
            o_set_audio_output_level(val);
            lua_settop(l, 0);
            return 0;
        }
    }
    fprintf(stderr, "wrong arguments for _set_audio_output_level; ");
    fprintf(stderr, "expects level(float)\n");
    lua_settop(l, 0);
    return 1;
}

int _set_audio_monitor_level(lua_State *l) {
    int nargs = lua_gettop(l);
    //fprintf(stderr, "set_audio_monitor_level nargs: %d\n", nargs);
    if(nargs == 1) {
        if( lua_isnumber(l, 1) ) {
            float val = lua_tonumber(l, 1);
            o_set_audio_monitor_level(val);
            lua_settop(l, 0);
            return 0;
        }
    }
    fprintf(stderr, "wrong arguments for _set_audio_monitor_level; ");
    fprintf(stderr, "expects level(float)\n");
    lua_settop(l, 0);
    return 1;
}

int _set_audio_monitor_mono(lua_State *l) {
    (void)l;
    o_set_audio_monitor_mono();
    return 0;
}

int _set_audio_monitor_stereo(lua_State *l) {
    (void)l;
    o_set_audio_monitor_stereo();
    return 0;
}

int _set_audio_monitor_on(lua_State *l) {
    (void)l;
    o_set_audio_monitor_on();
    return 0;
}

int _set_audio_monitor_off(lua_State *l) {
    (void)l;
    o_set_audio_monitor_off();
    return 0;
}

int _set_audio_pitch_on(lua_State *l) {
    (void)l;
    o_set_audio_pitch_on();
    return 0;
}

int _set_audio_pitch_off(lua_State *l) {
    (void)l;
    o_set_audio_pitch_off();
    return 0;
}

int _restart_audio(lua_State *l) {
    (void)l;
    o_restart_audio();
    return 0;
}
