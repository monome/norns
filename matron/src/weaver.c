/***
 * weaver
 * c->lua and lua->c interface
 * @module system
 */

// standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
#include "device_crow.h"
#include "events.h"
#include "hello.h"
#include "lua_eval.h"
#include "metro.h"
#include "screen.h"
#include "snd_file.h"
#include "i2c.h"
#include "osc.h"
#include "oracle.h"
#include "weaver.h"
#include "system_cmd.h"
#include "clock.h"
#include "clocks/clock_internal.h"
#include "clocks/clock_link.h"


// registered lua functions require the LVM state as a parameter.
// but often we don't need it.
// use pragma instead of casting to void as a workaround.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"


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
static int _grid_rows(lua_State *l);
static int _grid_cols(lua_State *l);
static int _grid_set_rotation(lua_State *l);

static int _arc_set_led(lua_State *l);
static int _arc_all_led(lua_State *l);
static int _monome_refresh(lua_State *l);
static int _monome_intensity(lua_State *l);

//screen
static int _screen_update(lua_State *l);
static int _screen_save(lua_State *l);
static int _screen_restore(lua_State *l);
static int _screen_font_face(lua_State *l);
static int _screen_font_size(lua_State *l);
static int _screen_aa(lua_State *l);
static int _screen_level(lua_State *l);
static int _screen_line_width(lua_State *l);
static int _screen_line_cap(lua_State *l);
static int _screen_line_join(lua_State *l);
static int _screen_miter_limit(lua_State *l);
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
static int _screen_text_extents(lua_State *l);
static int _screen_export_png(lua_State *l);
static int _screen_display_png(lua_State *l);
//i2c
static int _gain_hp(lua_State *l);
//osc
static int _osc_send(lua_State *l);
static int _osc_send_crone(lua_State *l);
// midi
static int _midi_send(lua_State *l);

// crow
static int _crow_send(lua_State *l);

// crone
/// engines
static int _request_engine_report(lua_State *l);
static int _load_engine(lua_State *l);
static int _free_engine(lua_State *l);
/// commands
static int _send_command(lua_State *l);
static int _start_poll(lua_State *l);
static int _stop_poll(lua_State *l);
static int _set_poll_time(lua_State *l);
static int _request_poll_value(lua_State *l);
// timing
static int _metro_start(lua_State *l);
static int _metro_stop(lua_State *l);
static int _metro_set_time(lua_State *l);
// get the current system time
static int _get_time(lua_State *l);
// usleep!
static int _micro_sleep(lua_State *l);

// audio context control
static int _set_level_dac(lua_State *l);
static int _set_level_adc(lua_State *l);
static int _set_level_ext(lua_State *l);
static int _set_level_tape(lua_State *l);
static int _set_level_monitor(lua_State *l);
static int _set_monitor_mix_mono(lua_State *l);
static int _set_monitor_mix_stereo(lua_State *l);

static int _set_audio_pitch_on(lua_State *l);
static int _set_audio_pitch_off(lua_State *l);

static int _poll_start_vu(lua_State *l);
static int _poll_stop_vu(lua_State *l);
static int _poll_start_cut_phase(lua_State *l);
static int _poll_stop_cut_phase(lua_State *l);

// tape control
static int _tape_rec_open(lua_State *l);
static int _tape_rec_start(lua_State *l);
static int _tape_rec_stop(lua_State *l);
static int _tape_play_open(lua_State *l);
static int _tape_play_start(lua_State *l);
static int _tape_play_stop(lua_State *l);

// cut
static int _set_level_adc_cut(lua_State *l);
static int _set_level_ext_cut(lua_State *l);
static int _set_level_tape_cut(lua_State *l);
static int _set_level_cut_rev(lua_State *l);
static int _set_level_cut_master(lua_State *l);
static int _set_level_cut(lua_State *l);
static int _set_level_cut_cut(lua_State *l);
static int _set_pan_cut(lua_State *l);
static int _cut_enable(lua_State *l);
static int _cut_buffer_clear(lua_State *l);
static int _cut_buffer_clear_channel(lua_State *l);
static int _cut_buffer_clear_region(lua_State *l);
static int _cut_buffer_clear_region_channel(lua_State *l);
static int _cut_buffer_read_mono(lua_State *l);
static int _cut_buffer_read_stereo(lua_State *l);
static int _cut_buffer_write_mono(lua_State *l);
static int _cut_buffer_write_stereo(lua_State *l);
static int _cut_reset(lua_State *l);
static int _set_cut_param(lua_State *l);
static int _set_cut_param_ii(lua_State *l);
static int _set_cut_param_iif(lua_State *l);
static int _set_level_input_cut(lua_State *l);

// rev effects controls
static int _set_rev_on(lua_State *l);
static int _set_rev_off(lua_State *l);
static int _set_level_monitor_rev(lua_State *l);
static int _set_level_ext_rev(lua_State *l);
static int _set_level_tape_rev(lua_State *l);
static int _set_level_rev_dac(lua_State *l);
static int _set_rev_param(lua_State *l);

// comp effects controls
static int _set_comp_on(lua_State *l);
static int _set_comp_off(lua_State *l);
static int _set_comp_mix(lua_State *l);
static int _set_comp_param(lua_State *l);

// start audio (sync with sclang startup)
static int _start_audio(lua_State *l);
// restart audio (recompile sclang)
static int _restart_audio(lua_State *l);

// soundfile inspection
static int _sound_file_inspect(lua_State *l);

// util
static int _system_cmd(lua_State *l);

// reset LVM
static int _reset_lvm(lua_State *l);
static int _clock_schedule_sleep(lua_State *l);
static int _clock_schedule_sync(lua_State *l);
static int _clock_cancel(lua_State *l);
static int _clock_internal_set_tempo(lua_State *l);

#if HAVE_ABLETON_LINK
static int _clock_link_set_tempo(lua_State *l);
static int _clock_link_set_quantum(lua_State *l);
#endif

static int _clock_set_source(lua_State *l);
static int _clock_get_time_beats(lua_State *l);
static int _clock_get_tempo(lua_State *l);

// boilerplate: push a function to the stack, from field in global 'norns'
static inline void
_push_norns_func(const char *field, const char *func) {
  // fprintf(stderr, "calling norns.%s.%s\n", field, func);
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, field);
  lua_remove(lvm, -2);
  lua_getfield(lvm, -1, func);
  lua_remove(lvm, -2);
}

#define lua_register_norns(n,f) \
              (lua_pushcfunction(lvm, f), lua_setfield(lvm, -2, n))

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
  
  // make table for global externs
  lua_newtable(lvm);

  // levels
  lua_register_norns("level_adc", &_set_level_adc);
  lua_register_norns("level_dac", &_set_level_dac);
  lua_register_norns("level_ext", &_set_level_ext);
  lua_register_norns("level_tape", &_set_level_tape);
  lua_register_norns("level_monitor", &_set_level_monitor);

  lua_register_norns("monitor_mix_mono", &_set_monitor_mix_mono);
  lua_register_norns("monitor_mix_stereo", &_set_monitor_mix_stereo);

  // fx
  lua_register_norns("rev_on", &_set_rev_on);
  lua_register_norns("rev_off", &_set_rev_off);
  lua_register_norns("rev_param", &_set_rev_param);
  lua_register_norns("level_ext_rev", &_set_level_ext_rev);
  lua_register_norns("level_rev_dac", &_set_level_rev_dac);
  lua_register_norns("level_monitor_rev", &_set_level_monitor_rev);
  lua_register_norns("level_tape_rev", &_set_level_tape_rev);
  lua_register_norns("comp_on", &_set_comp_on);
  lua_register_norns("comp_off", &_set_comp_off);
  lua_register_norns("comp_param", &_set_comp_param);
  lua_register_norns("comp_mix", &_set_comp_mix);

  // tape controls
  lua_register_norns("tape_record_open", &_tape_rec_open);
  lua_register_norns("tape_record_start", &_tape_rec_start);
  lua_register_norns("tape_record_stop", &_tape_rec_stop);
  lua_register_norns("tape_play_open", &_tape_play_open);
  lua_register_norns("tape_play_start", &_tape_play_start);
  lua_register_norns("tape_play_stop", &_tape_play_stop);

  // polls
  lua_register_norns("poll_start_vu", &_poll_start_vu);
  lua_register_norns("poll_stop_vu", &_poll_stop_vu);
  lua_register_norns("poll_start_cut_phase", &_poll_start_cut_phase);
  lua_register_norns("poll_stop_cut_phase", &_poll_stop_cut_phase);

  // cut
  lua_register_norns("level_adc_cut", &_set_level_adc_cut);
  lua_register_norns("level_ext_cut", &_set_level_ext_cut);
  lua_register_norns("level_tape_cut", &_set_level_tape_cut);
  lua_register_norns("level_cut_rev", &_set_level_cut_rev);
  lua_register_norns("level_cut_master", &_set_level_cut_master);
  lua_register_norns("level_cut", &_set_level_cut);
  lua_register_norns("level_cut_cut", &_set_level_cut_cut);
  lua_register_norns("pan_cut", &_set_pan_cut);
  lua_register_norns("cut_enable", &_cut_enable);
  lua_register_norns("cut_buffer_clear", &_cut_buffer_clear);
  lua_register_norns("cut_buffer_clear_channel", &_cut_buffer_clear_channel);
  lua_register_norns("cut_buffer_clear_region", &_cut_buffer_clear_region);
  lua_register_norns("cut_buffer_clear_region_channel", &_cut_buffer_clear_region_channel);
  lua_register_norns("cut_buffer_read_mono", &_cut_buffer_read_mono);
  lua_register_norns("cut_buffer_read_stereo", &_cut_buffer_read_stereo);
  lua_register_norns("cut_buffer_write_mono", &_cut_buffer_write_mono);
  lua_register_norns("cut_buffer_write_stereo", &_cut_buffer_write_stereo);
  lua_register_norns("cut_reset", &_cut_reset);
  lua_register_norns("cut_param", &_set_cut_param);
  lua_register_norns("cut_param_ii", &_set_cut_param_ii);
  lua_register_norns("cut_param_iif", &_set_cut_param_iif);
  lua_register_norns("level_input_cut", &_set_level_input_cut);

  // crow
  lua_register_norns("crow_send", &_crow_send);

  // util
  lua_register_norns("system_cmd", &_system_cmd);

  // low-level monome grid control
  lua_register_norns("grid_set_led", &_grid_set_led);
  lua_register_norns("grid_all_led", &_grid_all_led);
  lua_register_norns("grid_rows", &_grid_rows);
  lua_register_norns("grid_cols", &_grid_cols);
  lua_register_norns("grid_set_rotation", &_grid_set_rotation);
  lua_register_norns("arc_set_led", &_arc_set_led);
  lua_register_norns("arc_all_led", &_arc_all_led);
  lua_register_norns("monome_refresh", &_monome_refresh);
  lua_register_norns("monome_intensity", &_monome_intensity);

  // register screen funcs
  lua_register_norns("screen_update", &_screen_update);
  lua_register_norns("screen_save", &_screen_save);
  lua_register_norns("screen_restore", &_screen_restore);
  lua_register_norns("screen_font_face", &_screen_font_face);
  lua_register_norns("screen_font_size", &_screen_font_size);
  lua_register_norns("screen_aa", &_screen_aa);
  lua_register_norns("screen_level", &_screen_level);
  lua_register_norns("screen_line_width", &_screen_line_width);
  lua_register_norns("screen_line_cap", &_screen_line_cap);
  lua_register_norns("screen_line_join", &_screen_line_join);
  lua_register_norns("screen_miter_limit", &_screen_miter_limit);
  lua_register_norns("screen_move", &_screen_move);
  lua_register_norns("screen_line", &_screen_line);
  lua_register_norns("screen_move_rel", &_screen_move_rel);
  lua_register_norns("screen_line_rel", &_screen_line_rel);
  lua_register_norns("screen_curve", &_screen_curve);
  lua_register_norns("screen_curve_rel", &_screen_curve_rel);
  lua_register_norns("screen_arc", &_screen_arc);
  lua_register_norns("screen_rect", &_screen_rect);
  lua_register_norns("screen_stroke", &_screen_stroke);
  lua_register_norns("screen_fill", &_screen_fill);
  lua_register_norns("screen_text", &_screen_text);
  lua_register_norns("screen_clear", &_screen_clear);
  lua_register_norns("screen_close", &_screen_close);
  lua_register_norns("screen_text_extents", &_screen_text_extents);
  lua_register_norns("screen_export_png", &_screen_export_png);
  lua_register_norns("screen_display_png", &_screen_display_png);

  // analog output control
  lua_register_norns("gain_hp", &_gain_hp);

  // osc
  lua_register_norns("osc_send", &_osc_send);
  lua_register_norns("osc_send_crone", &_osc_send_crone);

  // midi
  lua_register_norns("midi_send", &_midi_send);

    // get list of available crone engines
  lua_register_norns("report_engines", &_request_engine_report);
  // load a named engine
  lua_register_norns("load_engine", &_load_engine);
  // free engine
  lua_register_norns("free_engine", &_free_engine);

  // send an indexed command
  lua_register_norns("send_command", &_send_command);

  // start/stop an indexed metro with callback
  lua_register_norns("metro_start", &_metro_start);
  lua_register_norns("metro_stop", &_metro_stop);
  lua_register_norns("metro_set_time", &_metro_set_time);

  // get the current high-resolution CPU time
  lua_register_norns("get_time", &_get_time);
  // usleep!
  lua_register_norns("usleep", &_micro_sleep);

  // start / stop a poll
  lua_register_norns("start_poll", &_start_poll);
  lua_register_norns("stop_poll", &_stop_poll);
  lua_register_norns("set_poll_time", &_set_poll_time);
  lua_register_norns("request_poll_value", &_request_poll_value);

  // audio context controls
  lua_register_norns("audio_pitch_on", &_set_audio_pitch_on);
  lua_register_norns("audio_pitch_off", &_set_audio_pitch_off);

  // start audio (query for sclang readiness)
  lua_register_norns("start_audio", &_start_audio);
  // restart the audio process (recompile sclang)
  lua_register_norns("restart_audio", &_restart_audio);

  // returns channels, frames, samplerate
  lua_register_norns("sound_file_inspect", &_sound_file_inspect);

  // reset LVM
  lua_register_norns("reset_lvm", &_reset_lvm);

  // clock
  lua_register_norns("clock_schedule_sleep", &_clock_schedule_sleep);
  lua_register_norns("clock_schedule_sync", &_clock_schedule_sync);
  lua_register_norns("clock_cancel", &_clock_cancel);
  lua_register_norns("clock_internal_set_tempo", &_clock_internal_set_tempo);
#if HAVE_ABLETON_LINK
  lua_register_norns("clock_link_set_tempo", &_clock_link_set_tempo);
  lua_register_norns("clock_link_set_quantum", &_clock_link_set_quantum);
#endif
  lua_register_norns("clock_set_source", &_clock_set_source);
  lua_register_norns("clock_get_time_beats", &_clock_get_time_beats);
  lua_register_norns("clock_get_tempo", &_clock_get_tempo);

  // name global extern table
  lua_setglobal(lvm, "_norns");


  // run system init code
  char *config = getenv("NORNS_CONFIG");
  char *home = getenv("HOME");
  char cmd[256];

  if (config == NULL) {
    snprintf(cmd, 256, "dofile('%s/norns/lua/core/config.lua')\n", home);
  } else {
    snprintf(cmd, 256, "dofile('%s')\n", config);
  }
  fprintf(stderr, "running lua config file: %s", cmd);
  w_run_code(cmd);
  w_run_code("require('core/norns')");
}

// run startup code
// audio backend should be running
void w_startup(void) {
  fprintf(stderr, "running startup\n");
  lua_getglobal(lvm, "_startup");
  l_report(lvm, l_docall(lvm, 0, 0));
}

void w_deinit(void) {
  fprintf(stderr, "shutting down lua vm\n");
  lua_close(lvm);
}

void w_reset_lvm() {
  w_deinit();
  w_init();
  w_startup();
}


//----------------------------------
//---- static definitions
//

#define STRING_NUM(n) #n
#define LUA_ARG_ERROR(n) "error: requires " STRING_NUM(n) " arguments" 
#define lua_check_num_args(n) \
  if (lua_gettop(l) != n) { return luaL_error(l, LUA_ARG_ERROR(n)); }

int _reset_lvm(lua_State *l) {
  lua_check_num_args(0);
  lua_settop(l, 0); 
  // do this through the event loop, not from inside a lua pcall
  event_post( event_data_new(EVENT_RESET_LVM) );
  return 0;
}


/***
 * screen: update (flip buffer)
 * @function s_update
 */
int _screen_update(lua_State *l) {
  lua_check_num_args(0);
  screen_update();
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: save attributes
 * @function s_save
 */
int _screen_save(lua_State *l) {
  lua_check_num_args(0);
  screen_save();
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: restore attributes
 * @function s_restore
 */
int _screen_restore(lua_State *l) {
  lua_check_num_args(0);
  screen_restore();
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: set font face
 * @function s_font_face
 */
int _screen_font_face(lua_State *l) {
  lua_check_num_args(1);
  int x = (int) luaL_checkinteger(l, 1) - 1;
  if(x<0) x = 0;
  screen_font_face(x);
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: set font size
 * @function s_font_size
 */
int _screen_font_size(lua_State *l) {
  lua_check_num_args(1);
  int x = (int) luaL_checknumber(l, 1);
  screen_font_size(x);
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: change antialias mode for drawing
 * @function s_aa
 * @tparam integer state, 0=off, 1=on
 */
int _screen_aa(lua_State *l) {
  lua_check_num_args(1);
  int x = (int) luaL_checkinteger(l, 1);
  screen_aa(x);
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: change level (color)
 * @function s_level
 * @tparam integer level, 0 (black) to 15 (white)
 */
int _screen_level(lua_State *l) {
  lua_check_num_args(1);
  int x = (int) luaL_checkinteger(l, 1);
  screen_level(x);
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: set line width
 * @function s_line_width
 * @tparam integer width line width
 */
int _screen_line_width(lua_State *l) {
  lua_check_num_args(1);
  double x = luaL_checknumber(l, 1);
  screen_line_width(x);
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: set line cap
 * @function s_line_cap
 * @tparam string line cap style ("butt", "round" or "square"). default is "butt".
 */
int _screen_line_cap(lua_State *l) {
  lua_check_num_args(1);
  const char *s = luaL_checkstring(l, 1);
  screen_line_cap(s);
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: set line join
 * @function s_line_join
 * @tparam string line join style ("miter", "round" or "bevel"). default is "miter".
 */
int _screen_line_join(lua_State *l) {
  lua_check_num_args(1);
  const char *s = luaL_checkstring(l, 1);
  screen_line_join(s);
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: set miter limit
 * @function s_miter_limit
 * @tparam double miter limit
 */
int _screen_miter_limit(lua_State *l) {
  lua_check_num_args(1);
  double limit = luaL_checknumber(l, 1);
  screen_miter_limit(limit);
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
  lua_check_num_args(2);
  double x = luaL_checknumber(l, 1);
  double y = luaL_checknumber(l, 2);
  screen_move(x,y);
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
  lua_check_num_args(2);
  double x = luaL_checknumber(l, 1);
  double y = luaL_checknumber(l, 2);
  screen_line(x,y);
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
  lua_check_num_args(2);
  double x = luaL_checknumber(l, 1);
  double y = luaL_checknumber(l, 2);
  screen_move_rel(x,y);
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
  lua_check_num_args(2);
  double x = (int) luaL_checknumber(l, 1);
  double y = (int) luaL_checknumber(l, 2);
  screen_line_rel(x,y);
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
  lua_check_num_args(6);
  double x1 = luaL_checknumber(l, 1);
  double y1 = luaL_checknumber(l, 2);
  double x2 = luaL_checknumber(l, 3);
  double y2 = luaL_checknumber(l, 4);
  double x3 = luaL_checknumber(l, 5);
  double y3 = luaL_checknumber(l, 6);
  screen_curve(x1, y1, x2, y2, x3, y3);
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
  lua_check_num_args(6);
  double x1 = luaL_checknumber(l, 1);
  double y1 = luaL_checknumber(l, 2);
  double x2 = luaL_checknumber(l, 3);
  double y2 = luaL_checknumber(l, 4);
  double x3 = luaL_checknumber(l, 5);
  double y3 = luaL_checknumber(l, 6);
  screen_curve_rel(x1, y1, x2, y2, x3, y3);
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
  lua_check_num_args(5);
  double x = luaL_checknumber(l, 1);
  double y = luaL_checknumber(l, 2);
  double r = luaL_checknumber(l, 3);
  double a1 = luaL_checknumber(l, 4);
  double a2 = luaL_checknumber(l, 5);
  screen_arc(x,y,r,a1,a2);
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
  lua_check_num_args(4);
  double x = luaL_checknumber(l, 1);
  double y = luaL_checknumber(l, 2);
  double w = luaL_checknumber(l, 3);
  double h = luaL_checknumber(l, 4);
  screen_rect(x, y, w, h);
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: stroke, or apply width/color to line(s)
 * @function s_stroke
 */
int _screen_stroke(lua_State *l) {
  lua_check_num_args(0);
  screen_stroke();
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: fill path
 * @function s_fill
 */
int _screen_fill(lua_State *l) {
  lua_check_num_args(0);
  screen_fill();
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: text
 * @function s_text
 * @tparam string text test to print
 */
int _screen_text(lua_State *l) {
  lua_check_num_args(1);
  const char *s = luaL_checkstring(l, 1);
  screen_text(s);
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: clear to black
 * @function s_clear
 */
int _screen_clear(lua_State *l) {
  lua_check_num_args(0);
  screen_clear();
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: close path
 * @function s_close
 */
int _screen_close(lua_State *l) {
  lua_check_num_args(0);
  screen_close_path();
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: text_extents
 * @function s_text_extents
 * @tparam gets x/y displacement of a string
 */
int _screen_text_extents(lua_State *l) {
  lua_check_num_args(1);
  const char *s = luaL_checkstring(l, 1);
  double *xy = screen_text_extents(s);
  lua_pushinteger(l, xy[0]);
  lua_pushinteger(l, xy[1]);
  return 2;
}

/***
 * screen: export_png
 * @function s_export_png
 * @tparam string filename
 */
int _screen_export_png(lua_State *l) {
  lua_check_num_args(1);
  const char *s = luaL_checkstring(l, 1);
  screen_export_png(s);
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: display_png
 * @function s_display_png
 * @tparam string filename
 */
int _screen_display_png(lua_State *l) {
  lua_check_num_args(3);
  const char *s = luaL_checkstring(l, 1);
  double x = luaL_checknumber(l, 2);
  double y = luaL_checknumber(l, 3);
  screen_display_png(s, x, y);
  lua_settop(l, 0);
  return 0;
}


/***
 * headphone: set level
 * @function gain_hp
 * @tparam integer level level (0-63)
 */
int _gain_hp(lua_State *l) {
  lua_check_num_args(1);
  int level = (int) luaL_checkinteger(l, 1);
  i2c_hp(level);
  lua_settop(l, 0);
  return 0;
}

/***
 * osc: send to arbitrary address
 * @function osc_send
 */
int _osc_send(lua_State *l) {
  const char *host = NULL;
  const char *port = NULL;
  const char *path = NULL;
  lo_message msg;

  int nargs = lua_gettop(l);

  // address
  luaL_checktype(l, 1, LUA_TTABLE);

  if (lua_rawlen(l, 1) != 2) {
    luaL_argerror(l, 1, "address should be a table in the form {host, port}");
  }

  lua_pushnumber(l, 1);
  lua_gettable(l, 1);
  if (lua_isstring(l, -1)) {
    host = lua_tostring(l, -1);
  } else {
    luaL_argerror(l, 1, "address should be a table in the form {host, port}");
  }
  lua_pop(l, 1);

  lua_pushnumber(l, 2);
  lua_gettable(l, 1);
  if (lua_isstring(l, -1)) {
    port = lua_tostring(l, -1);
  } else {
    luaL_argerror(l, 1, "address should be a table in the form {host, port}");
  }
  lua_pop(l, 1);

  // path
  luaL_checktype(l, 2, LUA_TSTRING);
  path = lua_tostring(l, 2);

  if ((host == NULL) || (port == NULL) || (path == NULL)) { return 1; }

  msg = lo_message_new();

  // add args (optional)
  if (nargs > 2) {
    luaL_checktype(l, 3, LUA_TTABLE);
    for (size_t i = 1; i <= lua_rawlen(l, 3); i++) {
      lua_pushnumber(l, i);
      lua_gettable(l, 3);
      int argtype = lua_type(l, -1);

      switch (argtype) {
      case LUA_TNIL:
	lo_message_add_nil(msg);
	break;
      case LUA_TNUMBER:
	lo_message_add_float(msg, lua_tonumber(l, -1));
	break;
      case LUA_TBOOLEAN:
	if (lua_toboolean(l, -1)) {
	  lo_message_add_true(msg);
	} else {
	  lo_message_add_false(msg);
	}
	break;
      case LUA_TSTRING:
	lo_message_add_string(msg, lua_tostring(l, -1));
	break;
      default:
	lo_message_free(msg);
	luaL_error(l, "invalid osc argument type %s",
		   lua_typename(l, argtype));
	break;
      } /* switch */

      lua_pop(l, 1);
    }
  }
  osc_send(host, port, path, msg);
  lo_message_free(msg);

  lua_settop(l, 0);
  return 0;
}


/***
 * osc: send to crone
 * @function osc_send
 */
int _osc_send_crone(lua_State *l) {
  const char *path = NULL;
  lo_message msg;

  int nargs = lua_gettop(l);

  // path
  luaL_checktype(l, 1, LUA_TSTRING);
  path = lua_tostring(l, 1);

  if (path == NULL) { return 1; }

  msg = lo_message_new();

  // add args (optional)
  if (nargs > 2) {
    luaL_checktype(l, 3, LUA_TTABLE);
    for (size_t i = 1; i <= lua_rawlen(l, 3); i++) {
      lua_pushnumber(l, i);
      lua_gettable(l, 3);
      int argtype = lua_type(l, -1);

      switch (argtype) {
      case LUA_TNIL:
	lo_message_add_nil(msg);
	break;
      case LUA_TNUMBER:
	lo_message_add_float(msg, lua_tonumber(l, -1));
	break;
      case LUA_TBOOLEAN:
	if (lua_toboolean(l, -1)) {
	  lo_message_add_true(msg);
	} else {
	  lo_message_add_false(msg);
	}
	break;
      case LUA_TSTRING:
	lo_message_add_string(msg, lua_tostring(l, -1));
	break;
      default:
	lo_message_free(msg);
	luaL_error(l, "invalid osc argument type %s",
		   lua_typename(l, argtype));
	break;
      } /* switch */

      lua_pop(l, 1);
    }
  }
  osc_send_crone(path, msg);
  lo_message_free(msg);

  lua_settop(l, 0);
  return 0;
}


/***
 * crow: send
 * @function _crow_send
 */
int _crow_send(lua_State *l) {
  struct dev_crow *d;
  const char *s;

  if (lua_gettop(l) != 2) {
    return luaL_error(l, "wrong number of arguments");
  }

  luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
  d = lua_touserdata(l, 1);
  s = luaL_checkstring(l, 2);
  lua_settop(l, 0);

  dev_crow_send(d, s);

  return 0;
}

/***
 * midi: send
 * @function midi_send
 */
int _midi_send(lua_State *l) {
  struct dev_midi *md;
  size_t nbytes;
  uint8_t *data;

  lua_check_num_args(2);

  luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
  md = lua_touserdata(l, 1);

  luaL_checktype(l, 2, LUA_TTABLE);
  nbytes = lua_rawlen(l, 2);
  data = malloc(nbytes);

  for (unsigned int i = 1; i <= nbytes; i++) {
    lua_pushinteger(l, i);
    lua_gettable(l, 2);

    // TODO: lua_isnumber
    data[i - 1] = lua_tointeger(l, -1);
    lua_pop(l, 1);
  }

  dev_midi_send(md, data, nbytes);
  free(data);

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
  lua_check_num_args(4);
  luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
  struct dev_monome *md = lua_touserdata(l, 1);
  int x = (int) luaL_checkinteger(l, 2) - 1; // convert from 1-base
  int y = (int) luaL_checkinteger(l, 3) - 1; // convert from 1-base
  int z = (int) luaL_checkinteger(l, 4); // don't convert value!
  dev_monome_grid_set_led(md, x, y, z);
  lua_settop(l, 0);
  return 0;
}

int _arc_set_led(lua_State *l) {
  lua_check_num_args(4);
  luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
  struct dev_monome *md = lua_touserdata(l, 1);
  int n = (int) luaL_checkinteger(l, 2) - 1; // convert from 1-base
  int x = (int) luaL_checkinteger(l, 3) - 1; // convert from 1-base
  int val = (int) luaL_checkinteger(l, 4); // don't convert value!
  dev_monome_arc_set_led(md, n, x, val);
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
  lua_check_num_args(2);
  luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
  struct dev_monome *md = lua_touserdata(l, 1);
  int z = (int) luaL_checkinteger(l, 2); // don't convert value!
  dev_monome_all_led(md, z);
  lua_settop(l, 0);
  return 0;
}

int _arc_all_led(lua_State *l) {
  return _grid_all_led(l);
}

/***
 * grid: set rotation
 * @param dev grid device
 * @param z (rotation 0-3 - or is it 0,90,180,270?)
 */
int _grid_set_rotation(lua_State *l) {
  lua_check_num_args(2);
  luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
  struct dev_monome *md = lua_touserdata(l, 1);
  int z = (int) luaL_checkinteger(l, 2); // don't convert value!
  dev_monome_set_rotation(md, z);
  lua_settop(l, 0);
  return 0;
}

/***
 * monome: refresh
 * @function monome_refresh
 * @param dev grid device
 */
int _monome_refresh(lua_State *l) {
  lua_check_num_args(1);
  luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
  struct dev_monome *md = lua_touserdata(l, 1);
  dev_monome_refresh(md);
  lua_settop(l, 0);
  return 0;
}

/***
 * monome: intensity
 * @function monome_intensity
 * @param dev device
 */
int _monome_intensity(lua_State *l) {
  lua_check_num_args(2);
  luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
  struct dev_monome *md = lua_touserdata(l, 1);
  int i = (int) luaL_checkinteger(l, 2); // don't convert value!
  dev_monome_intensity(md, i);
  lua_settop(l, 0);
  return 0;
}

/***
 * grid: rows
 * @function grid_rows
 * @param dev grid device
 */
int _grid_rows(lua_State *l) {
  lua_check_num_args(1);
  luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
  struct dev_monome *md = lua_touserdata(l, 1);
  lua_pushinteger(l, dev_monome_grid_rows(md));
  return 1;
}

/***
 * grid: cold
 * @function grid_cols
 * @param dev grid device
 */
int _grid_cols(lua_State *l) {
  lua_check_num_args(1);
  luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
  struct dev_monome *md = lua_touserdata(l, 1);
  lua_pushinteger(l, dev_monome_grid_cols(md));
  return 1;
}

//-- audio processing controls
int _load_engine(lua_State *l) {
  lua_check_num_args(1);
  const char *s = luaL_checkstring(l, 1);
  o_load_engine(s);
  lua_settop(l, 0);
  return 0;
}

int _free_engine(lua_State *l) {
  lua_check_num_args(0);
  o_free_engine();
  lua_settop(l, 0);
  return 0;
}

int _send_command(lua_State *l) {
  int nargs = lua_gettop(l);
  if (nargs < 1) {
    return luaL_error(l, "wrong number of arguments");
  }

  char *cmd = NULL;
  char *fmt = NULL;

  int idx = (int) luaL_checkinteger(l, 1) - 1; // 1-base to 0-base
  // FIXME: this isn't really safe.
  // to make it safe would mean locks, which is bad.
  // might be better to put name/fmt on stack from lua on every call
  cmd = o_get_commands()[idx].name;
  fmt = o_get_commands()[idx].format;

  // FIXME: refactor this, should go in oracle!
  lo_message msg = lo_message_new();

  const char *s;
  int d;
  double f;

  for(int i = 2; i <= nargs; i++) {
    switch(fmt[i - 2]) {
    case 's':
      if (lua_isstring(l, i)) {
	s = lua_tostring(l, i);
	lo_message_add_string(msg, s);
      } else {
	lo_message_free(msg);
	return luaL_error(l, "failed string type check");
      }
      break;
    case 'i':
      if (lua_isnumber(l, i)) {
	d =  (int)lua_tonumber(l, i);
	lo_message_add_int32(msg, d);
      } else {
	lo_message_free(msg);
	return luaL_error(l, "failed int type check");
      }
      break;
    case 'f':
      if (lua_isnumber(l, i)) {
	f = lua_tonumber(l, i);
	lo_message_add_float(msg, (float)f);
      } else {
	lo_message_free(msg);
	return luaL_error(l, "failed double type check");
      }
      break;
    default:
      break;
    } /* switch */
  }

  if ((cmd == NULL) || (fmt == NULL)) {
    lo_message_free(msg);
    return luaL_error(l, "null format/command string");
  }

  o_send_command(cmd, msg);
  lo_message_free(msg);
  lua_settop(l, 0);
  return 0;
}

int _request_engine_report(lua_State *l) {
  o_request_engine_report();
  return 0;
}

/***
 * metro: start
 * @function metro_start
 */
int _metro_start(lua_State *l) {
  static int idx = 0;
  double seconds = -1.0; // metro will re-use previous value
  int count = -1; // default: infinite
  int stage = 0;

  int nargs = lua_gettop(l);

  if (nargs > 0) {
    idx = (int) luaL_checkinteger(l, 1) - 1; // convert from 1-based
  }

  if (nargs > 1) {
    seconds = (double) luaL_checknumber(l, 2);
  }

  if (nargs > 2) {
    count = (int) luaL_checkinteger(l, 3);
  }

  if (nargs > 3) {
    stage = (int) luaL_checkinteger(l, 4) - 1; // convert from 1-based
  }

  metro_start(idx, seconds, count, stage);
  lua_settop(l, 0);
  return 0;
}

/***
 * metro: stop
 * @function metro_stop
 */
int _metro_stop(lua_State *l) {
  lua_check_num_args(1);
  int idx = (int) luaL_checkinteger(l, 1) - 1;
  metro_stop(idx);
  lua_settop(l, 0);
  return 0;
}

/***
 * metro: set time
 * @function metro_set_time
 */
int _metro_set_time(lua_State *l) {
  lua_check_num_args(2);
  int idx = (int) luaL_checkinteger(l, 1) - 1;
  float sec = (float) luaL_checknumber(l, 2);
  metro_set_time(idx, sec);
  lua_settop(l, 0);
  return 0;
}

/***
 * request current time since Epoch
 * @function get_time
 */
int _get_time(lua_State *l) {
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  // returns two results: microseconds, seconds
  lua_pushinteger(l, (lua_Integer)tv.tv_sec);
  lua_pushinteger(l, (lua_Integer)tv.tv_usec);
  return 2;
}



// usleep 
int _micro_sleep(lua_State *l) {
  lua_check_num_args(1);
  int usec = (float) luaL_checknumber(l, 1);
  usleep(usec);
  lua_settop(l, 0);
  return 0;
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
  l_report(lvm, l_docall(lvm, 4, 0));
}

int _clock_schedule_sleep(lua_State *l) {
  lua_check_num_args(2);
  int coro_id = (int) luaL_checkinteger(l, 1);
  double seconds = luaL_checknumber(l, 2);

  if (seconds == 0) {
    w_handle_clock_resume(coro_id);
  } else {
    clock_schedule_resume_sleep(coro_id, seconds);
  }

  return 0;
}

int _clock_schedule_sync(lua_State *l) {
  lua_check_num_args(2);
  int coro_id = (int) luaL_checkinteger(l, 1);
  double beats = luaL_checknumber(l, 2);

  if (beats == 0) {
    w_handle_clock_resume(coro_id);
  } else {
    clock_schedule_resume_sync(coro_id, beats);
  }

  return 0;
}

int _clock_cancel(lua_State *l) {
  lua_check_num_args(1);
  int coro_id = (int) luaL_checkinteger(l, 1);
  clock_cancel_coro(coro_id);
  return 0;
}

int _clock_internal_set_tempo(lua_State *l) {
  lua_check_num_args(1);
  double bpm = luaL_checknumber(l, 1);
  clock_internal_set_tempo(bpm);
  return 0;
}

#if HAVE_ABLETON_LINK
int _clock_link_set_tempo(lua_State *l) {
  lua_check_num_args(1);
  double bpm = luaL_checknumber(l, 1);
  clock_link_set_tempo(bpm);
  return 0;
}

int _clock_link_set_quantum(lua_State *l) {
  lua_check_num_args(1);
  double quantum = luaL_checknumber(l, 1);
  clock_link_set_quantum(quantum);
  return 0;
}
#endif

int _clock_set_source(lua_State *l) {
  lua_check_num_args(1);
  int source = (int) luaL_checkinteger(l, 1);
  clock_set_source(source);
  return 0;
}

int _clock_get_time_beats(lua_State *l) {
  lua_pushnumber(l, clock_gettime_beats());
  return 1;
}

int _clock_get_tempo(lua_State *l) {
  lua_pushnumber(l, clock_get_tempo());
  return 1;
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
  l_report(lvm, l_docall(lvm, 4, 0));
}

void w_handle_monome_remove(int id) {
  _push_norns_func("monome", "remove");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  l_report(lvm, l_docall(lvm, 1, 0));
}

void w_handle_grid_key(int id, int x, int y, int state) {
  _call_grid_handler(id, x, y, state > 0);
}

void w_handle_arc_encoder_delta(int id, int n, int delta) {
  _push_norns_func("arc", "delta");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  lua_pushinteger(lvm, n + 1);  // convert to 1-base
  lua_pushinteger(lvm, delta);
  l_report(lvm, l_docall(lvm, 3, 0));
}

void w_handle_arc_encoder_key(int id, int n, int state) {
  _push_norns_func("arc", "key");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  lua_pushinteger(lvm, n + 1);  // convert to 1-base
  lua_pushinteger(lvm, state);
  l_report(lvm, l_docall(lvm, 3, 0));
}

void w_handle_hid_add(void *p) {
  struct dev_hid *dev = (struct dev_hid *)p;
  struct dev_common *base = (struct dev_common *)p;
  int id = base->id;

  _push_norns_func("hid", "add");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
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

  lua_pushlightuserdata(lvm, dev);
  l_report(lvm, l_docall(lvm, 5, 0));
}

void w_handle_hid_remove(int id) {
  _push_norns_func("hid", "remove");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  l_report(lvm, l_docall(lvm, 1, 0));
}

void w_handle_hid_event(int id, uint8_t type, dev_code_t code, int value) {
  _push_norns_func("hid", "event");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  lua_pushinteger(lvm, type);
  lua_pushinteger(lvm, code);
  lua_pushinteger(lvm, value);
  l_report(lvm, l_docall(lvm, 4, 0));
}

void w_handle_crow_add(void *p) {
  struct dev_crow *dev = (struct dev_crow *)p;
  struct dev_common *base = (struct dev_common *)p;
  int id = base->id;

  _push_norns_func("crow", "add");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  lua_pushstring(lvm, base->name);
  lua_pushlightuserdata(lvm, dev);
  l_report(lvm, l_docall(lvm, 3, 0));
}

void w_handle_crow_remove(int id) {
  _push_norns_func("crow", "remove");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  l_report(lvm, l_docall(lvm, 1, 0));
}

void w_handle_crow_event(void *dev, int id) {
  struct dev_crow *d = (struct dev_crow *)dev;
  _push_norns_func("crow", "event");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  lua_pushstring(lvm, d->line);
  l_report(lvm, l_docall(lvm, 2, 0));
}

void w_handle_midi_add(void *p) {
  struct dev_midi *dev = (struct dev_midi *)p;
  struct dev_common *base = (struct dev_common *)p;
  int id = base->id;

  _push_norns_func("midi", "add");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  lua_pushstring(lvm, base->name);
  lua_pushlightuserdata(lvm, dev);
  l_report(lvm, l_docall(lvm, 3, 0));
}

void w_handle_midi_remove(int id) {
  _push_norns_func("midi", "remove");
  lua_pushinteger(lvm, id + 1); // convert to 1-base
  l_report(lvm, l_docall(lvm, 1, 0));
}

void w_handle_midi_event(int id, uint8_t *data, size_t nbytes) {
    _push_norns_func("midi", "event");
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    lua_createtable(lvm, nbytes, 0);
    for (size_t i = 0; i < nbytes; i++) {
        lua_pushinteger(lvm, data[i]);
        lua_rawseti(lvm, -2, i + 1);
    }
    l_report(lvm, l_docall(lvm, 2, 0));
}

void w_handle_osc_event(char *from_host,
                        char *from_port,
                        char *path,
                        lo_message msg) {
  const char *types = NULL;
  int argc;
  lo_arg **argv = NULL;

  types = lo_message_get_types(msg);
  argc = lo_message_get_argc(msg);
  argv = lo_message_get_argv(msg);

  _push_norns_func("osc", "event");

  lua_pushstring(lvm, path);

  lua_createtable(lvm, argc, 0);
  for (int i = 0; i < argc; i++) {
    switch (types[i]) {
    case LO_INT32:
      lua_pushinteger(lvm, argv[i]->i);
      break;
    case LO_FLOAT:
      lua_pushnumber(lvm, argv[i]->f);
      break;
    case LO_STRING:
      lua_pushstring(lvm, &argv[i]->s);
      break;
    case LO_BLOB:
      lua_pushlstring(lvm,
		      lo_blob_dataptr((lo_blob) argv[i]),
		      lo_blob_datasize((lo_blob) argv[i]));
      break;
    case LO_INT64:
      lua_pushinteger(lvm, argv[i]->h);
      break;
    case  LO_DOUBLE:
      lua_pushnumber(lvm, argv[i]->d);
      break;
    case LO_SYMBOL:
      lua_pushstring(lvm, &argv[i]->S);
      break;
    case LO_CHAR:
      lua_pushlstring(lvm, (const char *) &argv[i]->c, 1);
      break;
    case LO_MIDI:
      lua_pushlstring(lvm, (const char *) &argv[i]->m, 4);
      break;
    case LO_TRUE:
      lua_pushboolean(lvm, 1);
      break;
    case LO_FALSE:
      lua_pushboolean(lvm, 0);
      break;
    case LO_NIL:
      lua_pushnil(lvm);
      break;
    case LO_INFINITUM:
      lua_pushnumber(lvm, INFINITY);
      break;
    default:
      fprintf(stderr, "unknown osc typetag: %c\n", types[i]);
      lua_pushnil(lvm);
      break;
    } /* switch */
    lua_rawseti(lvm, -2, i + 1);
  }

  lua_createtable(lvm, 2, 0);
  lua_pushstring(lvm, from_host);
  lua_rawseti(lvm, -2, 1);
  lua_pushstring(lvm, from_port);
  lua_rawseti(lvm, -2, 2);

  l_report(lvm, l_docall(lvm, 3, 0));
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
  l_report(lvm, l_docall(lvm, 2, 0));
}

void w_handle_startup_ready_ok() {
  _push_norns_func("startup_status", "ok");
  l_report(lvm, l_docall(lvm, 0, 0));
}

void w_handle_startup_ready_timeout() {
  _push_norns_func("startup_status", "timeout");
  l_report(lvm, l_docall(lvm, 0, 0));
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
    if (p[i].type == POLL_TYPE_VALUE) {
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
  _push_norns_func("report", "commands");
  _push_commands();
  l_report(lvm, l_docall(lvm, 2, 0));

  _push_norns_func("report", "polls");
  _push_polls();
  l_report(lvm, l_docall(lvm, 2, 0));

  _push_norns_func("report", "did_engine_load");
  l_report(lvm, l_docall(lvm, 0, 0));
  // TODO
  // _push_params();
}

// metro handler
void w_handle_metro(const int idx, const int stage) {
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, "metro");
  lua_remove(lvm, -2);
  lua_pushinteger(lvm, idx + 1);   // convert to 1-based
  lua_pushinteger(lvm, stage + 1); // convert to 1-based
  l_report(lvm, l_docall(lvm, 2, 0));
}

// metro handler
void w_handle_clock_resume(const int coro_id) {
  lua_getglobal(lvm, "clock");
  lua_getfield(lvm, -1, "resume");
  lua_remove(lvm, -2);
  lua_pushinteger(lvm, coro_id);
  l_report(lvm, l_docall(lvm, 1, 0));
}

// gpio handler
void w_handle_key(const int n, const int val) {
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, "key");
  lua_remove(lvm, -2);
  lua_pushinteger(lvm, n);
  lua_pushinteger(lvm, val);
  l_report(lvm, l_docall(lvm, 2, 0));
}

// gpio handler
void w_handle_enc(const int n, const int delta) {
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, "enc");
  lua_remove(lvm, -2);
  lua_pushinteger(lvm, n);
  lua_pushinteger(lvm, delta);
  l_report(lvm, l_docall(lvm, 2, 0));
}

// system/battery
void w_handle_battery(const int percent, const int current) {
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, "battery");
  lua_remove(lvm, -2);
  lua_pushinteger(lvm, percent);
  lua_pushinteger(lvm, current);
  l_report(lvm, l_docall(lvm, 2, 0));
}

// system/power
void w_handle_power(const int present) {
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, "power");
  lua_remove(lvm, -2);
  lua_pushinteger(lvm, present);
  l_report(lvm, l_docall(lvm, 1, 0));
}

// stat
void w_handle_stat(const uint32_t disk, const uint16_t temp, const uint16_t cpu) {
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, "stat");
  lua_remove(lvm, -2);
  lua_pushinteger(lvm, disk);
  lua_pushinteger(lvm, temp);
  lua_pushinteger(lvm, cpu);
  l_report(lvm, l_docall(lvm, 3, 0));
}

void w_handle_poll_value(int idx, float val) {
  // fprintf(stderr, "_handle_poll_value: %d, %f\n", idx, val);
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, "poll");
  lua_remove(lvm, -2);
  lua_pushinteger(lvm, idx + 1); // convert to 1-base
  lua_pushnumber(lvm, val);
  l_report(lvm, l_docall(lvm, 2, 0));
}

void w_handle_poll_data(int idx, int size, uint8_t *data) {
  lua_getglobal(lvm, "_norns");
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
  l_report(lvm, l_docall(lvm, 2, 0));
}

/* void w_handle_poll_wave(int idx, uint8_t *data) { */
/*   // TODO */
/* } */

// argument is an array of 4 bytes
void w_handle_poll_io_levels(uint8_t *levels) {
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, "vu");
  lua_remove(lvm, -2);
  for(int i = 0; i < 4; ++i) {
    lua_pushinteger(lvm, levels[i]);
  }
  l_report(lvm, l_docall(lvm, 4, 0));
}

void w_handle_poll_softcut_phase(int idx, float val) {
  //fprintf(stderr, "_handle_poll_softcut_phase: %d, %f\n", idx, val);
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, "softcut_phase");
  lua_remove(lvm, -2);
  lua_pushinteger(lvm, idx + 1);
  lua_pushnumber(lvm, val);
  l_report(lvm, l_docall(lvm, 2, 0));
}

// handle system command capture
void w_handle_system_cmd(char *capture) {
  lua_getglobal(lvm, "_norns");
  lua_getfield(lvm, -1, "system_cmd_capture");
  lua_remove(lvm, -2);
  lua_pushstring(lvm, capture);
  l_report(lvm, l_docall(lvm, 1, 0));
}




// helper: set poll given by lua to given state
static int poll_set_state(lua_State *l, bool val) {
  lua_check_num_args(1);
  int idx = (int) luaL_checkinteger(l, 1) - 1; // convert from 1-based
  o_set_poll_state(idx, val);
  lua_settop(l, 0);
  return 0;
}

int _start_poll(lua_State *l) {
  return poll_set_state(l, true);
}

int _stop_poll(lua_State *l) {
  return poll_set_state(l, false);
}

int _set_poll_time(lua_State *l) {
  lua_check_num_args(2);
  int idx = (int) luaL_checkinteger(l, 1) - 1; // convert from 1-based
  float val = (float) luaL_checknumber(l, 2);
  o_set_poll_time(idx, val);
  lua_settop(l, 0);
  return 0;
}

int _request_poll_value(lua_State *l) {
  lua_check_num_args(1);
  int idx = (int) luaL_checkinteger(l, 1) - 1; // convert from 1-based
  o_request_poll_value(idx);
  lua_settop(l, 0);
  return 0;
}




// audio context control
int _set_level_adc(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_adc(val);
  lua_settop(l, 0);
  return 0;
}

int _set_level_dac(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_dac(val);
  lua_settop(l, 0);
  return 0;
}

int _set_level_ext(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_ext(val);
  lua_settop(l, 0);
  return 0;
}

int _set_level_monitor(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_monitor(val);
  lua_settop(l, 0);
  return 0;
}

int _set_monitor_mix_mono(lua_State *l) {
  o_set_monitor_mix_mono();
  return 0;
}

int _set_monitor_mix_stereo(lua_State *l) {
  o_set_monitor_mix_stereo();
  return 0;
}

int _set_audio_pitch_on(lua_State *l) {
  o_set_audio_pitch_on();
  return 0;
}

int _set_audio_pitch_off(lua_State *l) {
  o_set_audio_pitch_off();
  return 0;
}

int _set_level_tape(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_tape(val);
  lua_settop(l, 0);
  return 0;
}

int _tape_rec_open(lua_State *l) {
  lua_check_num_args(1);
  const char *s = luaL_checkstring(l, 1);
  o_tape_rec_open((char *) s);
  lua_settop(l, 0);
  return 0;
}

int _tape_rec_start(lua_State *l) {
  o_tape_rec_start();
  return 0;
}

int _tape_rec_stop(lua_State *l) {
  o_tape_rec_stop();
  return 0;
}

int _tape_play_open(lua_State *l) {
  lua_check_num_args(1);
  const char *s = luaL_checkstring(l, 1);
  o_tape_play_open((char *) s);
  lua_settop(l, 0);
  return 0;
}

int _tape_play_start(lua_State *l) {
  o_tape_play_start();
  return 0;
}

int _tape_play_stop(lua_State *l) {
  o_tape_play_stop();
  return 0;
}

int _poll_start_vu(lua_State *l) {
  o_poll_start_vu();
  return 0;
}

int _poll_stop_vu(lua_State *l) {
  o_poll_stop_vu();
  return 0;
}

int _poll_start_cut_phase(lua_State *l) {
  o_poll_start_cut_phase();
  return 0;
}

int _poll_stop_cut_phase(lua_State *l) {
  o_poll_stop_cut_phase();
  return 0;
}


int _cut_enable(lua_State *l) {
  lua_check_num_args(2);
  int idx = (int) luaL_checkinteger(l, 1) - 1;
  float val = (float) luaL_checknumber(l, 2);
  o_cut_enable(idx, val);
  return 0;
}

int _set_level_adc_cut(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_adc_cut(val);
  return 0;
}

int _set_level_ext_cut(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_ext_cut(val);
  return 0;
}

int _set_level_tape_cut(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_tape_cut(val);
  return 0;
}

int _set_level_cut_rev(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_cut_rev(val);
  return 0;
}

int _set_level_cut_master(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_cut_master(val);
  return 0;
}

int _set_level_cut(lua_State *l) {
  lua_check_num_args(2);
  int idx = (int) luaL_checkinteger(l, 1) - 1;
  float val = (float) luaL_checknumber(l, 2);
  o_set_level_cut(idx, val);
  return 0;
}

int _set_level_cut_cut(lua_State *l) {
  lua_check_num_args(3);
  int src = (int) luaL_checkinteger(l, 1) - 1;
  int dest = (int) luaL_checkinteger(l, 2) - 1;
  float val = (float) luaL_checknumber(l, 3);
  o_set_level_cut_cut(src, dest, val);
  return 0;
}

int _set_pan_cut(lua_State *l) {
  lua_check_num_args(2);
  int idx = (int) luaL_checkinteger(l, 1) - 1;
  float val = (float) luaL_checknumber(l, 2);
  o_set_pan_cut(idx, val);
  return 0;
}

int _cut_buffer_clear(lua_State *l) {
  o_cut_buffer_clear();
  return 0;
}

int _cut_buffer_clear_channel(lua_State *l) {
  lua_check_num_args(1);
  int ch = (int) luaL_checkinteger(l, 1) - 1;
  o_cut_buffer_clear_channel(ch);
  return 0;
}

int _cut_buffer_clear_region(lua_State *l) {
  lua_check_num_args(2);
  float start = (float) luaL_checknumber(l, 1);
  float dur = (float) luaL_checknumber(l, 2);
  o_cut_buffer_clear_region(start, dur);
  return 0;
}

int _cut_buffer_clear_region_channel(lua_State *l) {
  lua_check_num_args(3);
  int ch = (int) luaL_checkinteger(l, 1) - 1;
  float start = (float) luaL_checknumber(l, 2);
  float dur = (float) luaL_checknumber(l, 3);
  o_cut_buffer_clear_region_channel(ch, start, dur);
  return 0;
}

int _cut_buffer_read_mono(lua_State *l) {
  lua_check_num_args(6);
  const char *s = luaL_checkstring(l, 1);
  float start_src = (float) luaL_checknumber(l, 2);
  float start_dst = (float) luaL_checknumber(l, 3);
  float dur = (float) luaL_checknumber(l, 4);
  int ch_src = (int) luaL_checkinteger(l, 5) - 1;
  int ch_dst = (int) luaL_checkinteger(l, 6) - 1;
  o_cut_buffer_read_mono((char *)s, start_src, start_dst, dur, ch_src, ch_dst);
  return 0;
}

int _cut_buffer_read_stereo(lua_State *l) {
  lua_check_num_args(4);
  const char *s = luaL_checkstring(l, 1);
  float start_src = (float) luaL_checknumber(l, 2);
  float start_dst = (float) luaL_checknumber(l, 3);
  float dur = (float) luaL_checknumber(l, 4);
  o_cut_buffer_read_stereo((char *)s, start_src, start_dst, dur);
  return 0;
}

int _cut_buffer_write_mono(lua_State *l) {
  lua_check_num_args(4);
  const char *s = luaL_checkstring(l, 1);
  float start = (float) luaL_checknumber(l, 2);
  float dur = (float) luaL_checknumber(l, 3);
  int ch = (int) luaL_checkinteger(l, 4) - 1;
  o_cut_buffer_write_mono((char *)s, start, dur, ch);
  return 0;
}

int _cut_buffer_write_stereo(lua_State *l) {
  lua_check_num_args(3);
  const char *s = luaL_checkstring(l, 1);
  float start = (float) luaL_checknumber(l, 2);
  float dur = (float) luaL_checknumber(l, 3);
  o_cut_buffer_write_stereo((char *)s, start, dur);
  return 0;
}

int _cut_reset(lua_State *l) {
  o_cut_reset();
  return 0;
}

int _set_cut_param(lua_State *l) {
  lua_check_num_args(3);
  const char *s = luaL_checkstring(l, 1);
  int voice = (int) luaL_checkinteger(l, 2) - 1;
  float val = (float) luaL_checknumber(l, 3);
  o_set_cut_param((char *)s, voice, val);
  return 0;
}

int _set_cut_param_ii(lua_State *l) {
  lua_check_num_args(3);
  const char *s = luaL_checkstring(l, 1);
  int voice = (int) luaL_checkinteger(l, 2) - 1;
  float val = (int) luaL_checkinteger(l, 3) - 1;
  o_set_cut_param_ii((char *)s, voice, val);
  return 0;
}

int _set_cut_param_iif(lua_State *l) {
  lua_check_num_args(4);
  const char *s = luaL_checkstring(l, 1);
  int a = (int) luaL_checkinteger(l, 2);
  int b = (int) luaL_checkinteger(l, 3);
  float val = (float) luaL_checknumber(l, 4);
  o_set_cut_param_iif((char *)s, a, b, val);
  return 0;
}

int _set_level_input_cut(lua_State *l) {
  lua_check_num_args(3);
  int ch = (int) luaL_checkinteger(l, 1) - 1;
  int voice = (int) luaL_checkinteger(l, 2) - 1;
  float val = (float) luaL_checknumber(l, 3);
  o_set_level_input_cut(ch, voice, val);
  return 0;
}




// rev effects controls
int _set_rev_on(lua_State *l) {
  o_set_rev_on();
  return 0;
}

int _set_rev_off(lua_State *l) {
  o_set_rev_off();
  return 0;
}

int _set_level_monitor_rev(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_monitor_rev(val);
  return 0;
}

int _set_level_ext_rev(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_ext_rev(val);
  return 0;
}

int _set_level_tape_rev(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_tape_rev(val);
  return 0;
}

int _set_level_rev_dac(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_level_rev_dac(val);
  return 0;
}

int _set_rev_param(lua_State *l) {
  lua_check_num_args(2);
  const char *s = luaL_checkstring(l, 1);
  float val = (float) luaL_checknumber(l, 2);
  o_set_rev_param(s, val);
  return 0;
}


// comp effects controls
int _set_comp_on(lua_State *l) {
  o_set_comp_on();
  return 0;
}

int _set_comp_off(lua_State *l) {
  o_set_comp_off();
  return 0;
}

int _set_comp_mix(lua_State *l) {
  lua_check_num_args(1);
  float val = (float) luaL_checknumber(l, 1);
  o_set_comp_mix(val);
  return 0;
}

int _set_comp_param(lua_State *l) {
  lua_check_num_args(2);
  const char *s = luaL_checkstring(l, 1);
    float val = (float) luaL_checknumber(l, 2);
  o_set_comp_param(s, val);
  return 0;
}

int _start_audio(lua_State *l) {
  norns_hello_start();
  return 0;
}

int _restart_audio(lua_State *l) {
  o_restart_audio();
  norns_hello_start();
  return 0;
}

int _sound_file_inspect(lua_State *l) {
  lua_check_num_args(1);
  const char *path = luaL_checkstring(l, 1);
  struct snd_file_desc desc = snd_file_inspect(path);
  lua_pushinteger(l, desc.channels);
  lua_pushinteger(l, desc.frames);
  lua_pushinteger(l, desc.samplerate);
  return 3;
}

int _system_cmd(lua_State *l)
{
  lua_check_num_args(1);
  const char *cmd = luaL_checkstring(l, 1);
  system_cmd((char *)cmd);
  return 0;
}

#pragma GCC diagnostic pop
