/***
 * weaver
 * c->lua and lua->c interface
 * @module system
 */

// standard
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// linux / posix
#include <glob.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

// lua
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

// norns
#include "clock.h"
#include "clocks/clock_crow.h"
#include "clocks/clock_internal.h"
#include "clocks/clock_link.h"
#include "clocks/clock_scheduler.h"
#include "device.h"
#include "device_list.h"
#include "device_crow.h"
#include "device_hid.h"
#include "device_midi.h"
#include "device_monome.h"
#include "device_serial.h"
#include "events.h"
#include "event_custom.h"
#include "hello.h"
#include "i2c.h"
#include "jack_client.h"
#include "lua_eval.h"
#include "metro.h"
#include "oracle.h"
#include "osc.h"
#include "platform.h"
#include "screen.h"
#include "screen_events.h"
#include "screen_results.h"
#include "snd_file.h"
#include "system_cmd.h"
#include "time_since.h"
#include "weaver.h"

// registered lua functions require the LVM state as a parameter.
// but often we don't need it.
// use pragma instead of casting to void as a workaround.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

//------
//---- global lua state!
static lua_State *lvm;

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
// to avoid shadowing similar-named extern functions in other modules (like
// screen)
// and also to distinguish from extern 'w_' functions.

// grid
static int _grid_set_led(lua_State *l);
static int _grid_all_led(lua_State *l);
static int _grid_rows(lua_State *l);
static int _grid_cols(lua_State *l);
static int _grid_set_rotation(lua_State *l);
static int _grid_tilt_enable(lua_State *l);
static int _grid_tilt_disable(lua_State *l);

static int _arc_set_led(lua_State *l);
static int _arc_all_led(lua_State *l);
static int _monome_refresh(lua_State *l);
static int _monome_intensity(lua_State *l);

// screen
static int _screen_update(lua_State *l);
static int _screen_save(lua_State *l);
static int _screen_restore(lua_State *l);
static int _screen_font_face(lua_State *l);
static int _screen_font_size(lua_State *l);
static int _screen_aa(lua_State *l);
static int _screen_gamma(lua_State *l);
static int _screen_brightness(lua_State*l);
static int _screen_contrast(lua_State*l);
static int _screen_invert(lua_State *l);
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
static int _screen_text_right(lua_State *l);
static int _screen_text_center(lua_State *l);
static int _screen_text_extents(lua_State *l);
static int _screen_text_trim(lua_State *l);
static int _screen_clear(lua_State *l);
static int _screen_close(lua_State *l);
static int _screen_export_png(lua_State *l);
static int _screen_export_screenshot(lua_State *l);
static int _screen_display_png(lua_State *l);
static int _screen_peek(lua_State *l);
static int _screen_poke(lua_State *l);
static int _screen_rotate(lua_State *l);
static int _screen_translate(lua_State *l);
static int _screen_set_operator(lua_State *l);

// image
typedef struct {
    screen_surface_t *surface;
    screen_context_t *context;
    const screen_context_t *previous_context;
    char *name;
} _image_t;

static luaL_Reg _image_methods[];
static luaL_Reg _image_functions[];
static const char *_image_class_name = "norns.image";

static int _image_new(lua_State *l, screen_surface_t *surface, const char *name);
static _image_t *_image_check(lua_State *l, int arg);
static int _image_context_focus(lua_State *l);
static int _image_context_defocus(lua_State *l);
static int _image_free(lua_State *l);
static int _image_equals(lua_State *l);
static int _image_tostring(lua_State *l);
static int _image_extents(lua_State *l);
static int _image_name(lua_State *l);
static int _screen_load_png(lua_State *l);
static int _screen_create_image(lua_State *l);
static int _screen_current_point(lua_State *l);
static int _screen_display_image(lua_State *l);
static int _screen_display_image_region(lua_State *l);

// i2c
static int _gain_hp(lua_State *l);
static int _adc_rev(lua_State *l);

// osc
static int _osc_send(lua_State *l);
static int _osc_send_crone(lua_State *l);

// midi
static int _midi_send(lua_State *l);
static int _midi_clock_receive(lua_State *l);

// crow
static int _crow_send(lua_State *l);

// serial
static int _serial_send(lua_State *l);

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
static int _cut_buffer_copy_mono(lua_State *l);
static int _cut_buffer_copy_stereo(lua_State *l);
static int _cut_buffer_read_mono(lua_State *l);
static int _cut_buffer_read_stereo(lua_State *l);
static int _cut_buffer_write_mono(lua_State *l);
static int _cut_buffer_write_stereo(lua_State *l);
static int _cut_buffer_render(lua_State *l);
static int _cut_query_position(lua_State *l);
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
static int _system_glob(lua_State *l);

// clock
static int _clock_schedule_sleep(lua_State *l);
static int _clock_schedule_sync(lua_State *l);
static int _clock_cancel(lua_State *l);
static int _clock_internal_set_tempo(lua_State *l);
static int _clock_internal_start(lua_State *l);
static int _clock_internal_stop(lua_State *l);
static int _clock_crow_in_div(lua_State *l);
#if HAVE_ABLETON_LINK
static int _clock_link_set_tempo(lua_State *l);
static int _clock_link_set_quantum(lua_State *l);
static int _clock_link_set_transport_stop(lua_State *l);
static int _clock_link_set_transport_start(lua_State *l);
static int _clock_link_set_start_stop_sync(lua_State *l);
static int _clock_link_get_number_of_peers(lua_State *l);
#endif
static int _clock_set_source(lua_State *l);
static int _clock_get_time_beats(lua_State *l);
static int _clock_get_tempo(lua_State *l);

// audio performance
static int _audio_get_cpu_load(lua_State *l);
static int _audio_get_xrun_count(lua_State *l);

// time-since measurement
static int _cpu_time_start_timer(lua_State *l);
static int _cpu_time_get_delta(lua_State *l);
static int _wall_time_start_timer(lua_State *l);
static int _wall_time_get_delta(lua_State *l);

// platform detection (CM3 vs PI3 vs OTHER)
static int _platform(lua_State *l);

// boilerplate: push a lua function to the lua stack, from named field in global 'norns'
static inline void _push_norns_func(const char *field, const char *func) {
    lua_getglobal(lvm, "_norns");
    lua_getfield(lvm, -1, field);
    lua_remove(lvm, -2);
    lua_getfield(lvm, -1, func);
    lua_remove(lvm, -2);
}

// boilerplate: push a C function to the lua stack
static inline void lua_register_norns(const char *name, int (*f)(lua_State *l)) {
    lua_pushcfunction(lvm, f), lua_setfield(lvm, -2, name);
}

static void lua_register_norns_class(const char *class_name, const luaL_Reg *methods, const luaL_Reg *functions) {
    // create the class metatable
    luaL_newmetatable(lvm, class_name);
    lua_pushstring(lvm, "__index");

    // build the table which will become __index
    lua_newtable(lvm);

    // insert class name in __index to help debugging
    lua_pushstring(lvm, "class");
    lua_pushstring(lvm, class_name);
    lua_rawset(lvm, -3);

    // insert methods starting with __ in metatable, else __index table
    for (const luaL_Reg *method = methods; method->name; method++) {
        lua_pushstring(lvm, method->name);
        lua_pushcfunction(lvm, method->func);
        lua_rawset(lvm, strncmp("__", method->name, 2) == 0 ? -5 : -3);
    }

    // insert the built __index table into the metatable and pop the metatable
    lua_rawset(lvm, -3);
    lua_pop(lvm, 1);

    // register any module level functions
    for (const luaL_Reg *function = functions; function->name; function++) {
        lua_register_norns(function->name, function->func);
    }
}

////////////////////////////////
//// extern function definitions

void w_init(void) {
    fprintf(stderr, "starting main lua vm\n");
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
    lua_register_norns("cut_buffer_copy_mono", &_cut_buffer_copy_mono);
    lua_register_norns("cut_buffer_copy_stereo", &_cut_buffer_copy_stereo);
    lua_register_norns("cut_buffer_read_mono", &_cut_buffer_read_mono);
    lua_register_norns("cut_buffer_read_stereo", &_cut_buffer_read_stereo);
    lua_register_norns("cut_buffer_write_mono", &_cut_buffer_write_mono);
    lua_register_norns("cut_buffer_write_stereo", &_cut_buffer_write_stereo);
    lua_register_norns("cut_buffer_render", &_cut_buffer_render);
    lua_register_norns("cut_query_position", &_cut_query_position);
    lua_register_norns("cut_reset", &_cut_reset);
    lua_register_norns("cut_param", &_set_cut_param);
    lua_register_norns("cut_param_ii", &_set_cut_param_ii);
    lua_register_norns("cut_param_iif", &_set_cut_param_iif);
    lua_register_norns("level_input_cut", &_set_level_input_cut);

    // crow
    lua_register_norns("crow_send", &_crow_send);

    // serial
    lua_register_norns("serial_send", &_serial_send);

    // util
    lua_register_norns("system_cmd", &_system_cmd);
    lua_register_norns("system_glob", &_system_glob);

    // low-level monome grid control
    lua_register_norns("grid_set_led", &_grid_set_led);
    lua_register_norns("grid_all_led", &_grid_all_led);
    lua_register_norns("grid_rows", &_grid_rows);
    lua_register_norns("grid_cols", &_grid_cols);
    lua_register_norns("grid_set_rotation", &_grid_set_rotation);
    lua_register_norns("grid_tilt_enable", &_grid_tilt_enable);
    lua_register_norns("grid_tilt_disable", &_grid_tilt_disable);
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
    lua_register_norns("screen_gamma", &_screen_gamma);
    lua_register_norns("screen_brightness", &_screen_brightness);
    lua_register_norns("screen_contrast", &_screen_contrast);
    lua_register_norns("screen_invert", &_screen_invert);
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
    lua_register_norns("screen_text_right", &_screen_text_right);
    lua_register_norns("screen_text_center", &_screen_text_center);
    lua_register_norns("screen_text_extents", &_screen_text_extents);
    lua_register_norns("screen_text_trim", &_screen_text_trim);    
    
    lua_register_norns("screen_clear", &_screen_clear);
    lua_register_norns("screen_close", &_screen_close);
    
    lua_register_norns("screen_export_png", &_screen_export_png);
    lua_register_norns("screen_export_screenshot", &_screen_export_screenshot);
    lua_register_norns("screen_display_png", &_screen_display_png);
    lua_register_norns("screen_peek", &_screen_peek);
    lua_register_norns("screen_poke", &_screen_poke);
    lua_register_norns("screen_rotate", &_screen_rotate);
    lua_register_norns("screen_translate", &_screen_translate);
    lua_register_norns("screen_set_operator", &_screen_set_operator);
    lua_register_norns("screen_current_point", &_screen_current_point);
    
    // image
    lua_register_norns_class(_image_class_name, _image_methods, _image_functions);

    // analog output control
    lua_register_norns("gain_hp", &_gain_hp);
    lua_register_norns("adc_rev", &_adc_rev);

    // osc
    lua_register_norns("osc_send", &_osc_send);
    lua_register_norns("osc_send_crone", &_osc_send_crone);

    // midi
    lua_register_norns("midi_send", &_midi_send);
    lua_register_norns("midi_clock_receive", &_midi_clock_receive);

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

    // clock
    lua_register_norns("clock_schedule_sleep", &_clock_schedule_sleep);
    lua_register_norns("clock_schedule_sync", &_clock_schedule_sync);
    lua_register_norns("clock_cancel", &_clock_cancel);
    lua_register_norns("clock_internal_set_tempo", &_clock_internal_set_tempo);
    lua_register_norns("clock_internal_start", &_clock_internal_start);
    lua_register_norns("clock_internal_stop", &_clock_internal_stop);
    lua_register_norns("clock_crow_in_div", &_clock_crow_in_div);
#if HAVE_ABLETON_LINK
    lua_register_norns("clock_link_set_tempo", &_clock_link_set_tempo);
    lua_register_norns("clock_link_set_quantum", &_clock_link_set_quantum);
    lua_register_norns("clock_link_get_number_of_peers", &_clock_link_get_number_of_peers);
    lua_register_norns("clock_link_set_transport_start", &_clock_link_set_transport_start);
    lua_register_norns("clock_link_set_transport_stop", &_clock_link_set_transport_stop);
    lua_register_norns("clock_link_set_start_stop_sync", &_clock_link_set_start_stop_sync);
#endif
    lua_register_norns("clock_set_source", &_clock_set_source);
    lua_register_norns("clock_get_time_beats", &_clock_get_time_beats);
    lua_register_norns("clock_get_tempo", &_clock_get_tempo);

    lua_register_norns("audio_get_cpu_load", &_audio_get_cpu_load);
    lua_register_norns("audio_get_xrun_count", &_audio_get_xrun_count);

    lua_register_norns("cpu_time_start_timer", &_cpu_time_start_timer);
    lua_register_norns("cpu_time_get_delta", &_cpu_time_get_delta);
    lua_register_norns("wall_time_start_timer", &_wall_time_start_timer);
    lua_register_norns("wall_time_get_delta", &_wall_time_get_delta);

    // platform
    lua_register_norns("platform", &_platform);

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
    lua_getglobal(lvm, "_startup");
    l_report(lvm, l_docall(lvm, 0, 0));
}

void w_post_startup(void) {
    lua_getglobal(lvm, "_post_startup");
    l_report(lvm, l_docall(lvm, 0, 0));
}

void w_deinit(void) {
    fprintf(stderr, "shutting down lua vm\n");
    lua_close(lvm);
}

//----------------------------------
//---- static definitions
//

#define STRING_NUM(n) #n
#define LUA_ARG_ERROR(n) "error: requires " STRING_NUM(n) " arguments"
#define lua_check_num_args(n)                   \
    if (lua_gettop(l) != n) {                   \
        return luaL_error(l, LUA_ARG_ERROR(n)); \
    }

//--------------------------------
//--- screen

/***
 * screen: update (flip buffer)
 * @function s_update
 */
int _screen_update(lua_State *l) {
  lua_check_num_args(0);
  screen_event_update();
  lua_settop(l, 0);
  return 0;
}

/***
 * screen: save attributes
 * @function s_save
 */
int _screen_save(lua_State *l) {
    lua_check_num_args(0);
    screen_event_save();
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: restore attributes
 * @function s_restore
 */
int _screen_restore(lua_State *l) {
    lua_check_num_args(0);
    screen_event_restore();
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: set font face
 * @function s_font_face
 */
int _screen_font_face(lua_State *l) {
    lua_check_num_args(1);
    int x = (int)luaL_checkinteger(l, 1) - 1;
    if (x < 0)
        x = 0;
    screen_event_font_face(x);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: set font size
 * @function s_font_size
 */
int _screen_font_size(lua_State *l) {
    lua_check_num_args(1);
    int x = (int)luaL_checknumber(l, 1);
    screen_event_font_size(x);
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
    int x = (int)luaL_checkinteger(l, 1);
    screen_event_aa(x);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: change gamma curve for drawing
 * @function s_gamma
 * @tparam double gamma, 0.0 <
 */
int _screen_gamma(lua_State *l) {
    lua_check_num_args(1);
    double g = luaL_checknumber(l, 1);
    screen_event_gamma(g);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: change pre-charge voltage for drawing
 * @function s_brightness
 * @tparam int level, [0, 15]
 */
int _screen_brightness(lua_State *l) {
    lua_check_num_args(1);
    int b = luaL_checkinteger(l, 1);
    screen_event_brightness(b);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: change contrast level of screen
 * @function s_contrast
 * @tparam int level, [0, 255]
 */
int _screen_contrast(lua_State *l) {
    lua_check_num_args(1);
    int c = luaL_checkinteger(l, 1);
    screen_event_contrast(c);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: invert the screen's pixels, it is not
 * required to call screen_update() to take effect.
 * @function s_invert
 */
int _screen_invert(lua_State *l) {
    lua_check_num_args(1);
    int i = luaL_checkinteger(l, 1);
    screen_event_invert(i);
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
    int x = (int)luaL_checkinteger(l, 1);
    screen_event_level(x);
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
    screen_event_line_width(x);
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
    screen_event_line_cap(s);
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
    screen_event_line_join(s);
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
    screen_event_miter_limit(limit);
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
    screen_event_move(x, y);
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
    screen_event_line(x, y);
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
    screen_event_move_rel(x, y);
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
    double x = (int)luaL_checknumber(l, 1);
    double y = (int)luaL_checknumber(l, 2);
    screen_event_line_rel(x, y);
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
    screen_event_curve(x1, y1, x2, y2, x3, y3);
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
    screen_event_curve_rel(x1, y1, x2, y2, x3, y3);
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
    screen_event_arc(x, y, r, a1, a2);
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
    screen_event_rect(x, y, w, h);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: stroke, or apply width/color to line(s)
 * @function s_stroke
 */
int _screen_stroke(lua_State *l) {
    lua_check_num_args(0);
    screen_event_stroke();
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: fill path
 * @function s_fill
 */
int _screen_fill(lua_State *l) {
    lua_check_num_args(0);
    screen_event_fill();
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
    screen_event_text(s);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: right-justified text
 */
int _screen_text_right(lua_State *l) {
    lua_check_num_args(1);
    const char *s = luaL_checkstring(l, 1);
    screen_event_text_right(s);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: center-justified text
 */
int _screen_text_center(lua_State *l) {
    lua_check_num_args(1);
    const char *s = luaL_checkstring(l, 1);
    screen_event_text_center(s);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: text trimmed to width
 */
int _screen_text_trim(lua_State *l) {
    lua_check_num_args(2);
    const char *s = luaL_checkstring(l, 1);
    double w = luaL_checknumber(l, 2);
    screen_event_text_trim(s, w);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: clear to black
 * @function s_clear
 */
int _screen_clear(lua_State *l) {
    lua_check_num_args(0);
    screen_event_clear();
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: close path
 * @function s_close
 */
int _screen_close(lua_State *l) {
    lua_check_num_args(0);
    screen_event_close_path();
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
    
    screen_event_text_extents(s);
    screen_results_wait();
    union screen_results_data *data = screen_results_get();
#if 1 // legacy API
    lua_pushinteger(l, (int)data->text_extents.width);
    lua_pushinteger(l, (int)data->text_extents.height);
    screen_results_free();
    return 2;
#else
    lua_pushnumber(l, data->text_extents.x_bearing);
    lua_pushnumber(l, data->text_extents.y_bearing);
    lua_pushnumber(l, data->text_extents.width);
    lua_pushnumber(l, data->text_extents.height);
    lua_pushnumber(l, data->text_extents.x_advance);
    lua_pushnumber(l, data->text_extents.y_advance);
    event_data_free(ev);
    return 6;
#endif

}

/***
 * screen: export_png
 * @function s_export_png 
 * @tparam string filename
 */
int _screen_export_png(lua_State *l) {
    lua_check_num_args(1);
    const char *s = luaL_checkstring(l, 1);
    screen_event_export_png(s);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: export_screenshot
 * @function s_export_screenshot
 * @tparam string filename
 */
int _screen_export_screenshot(lua_State *l) {
    lua_check_num_args(1);
    const char *s = luaL_checkstring(l, 1);
    screen_export_screenshot(s);
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
    screen_event_display_png(s, x, y);
    lua_settop(l, 0);
    return 0;
}


/***
 * screen: peek
 * @function s_peek
 * @tparam integer x screen x position (0-127)
 * @tparam integer y screen y position (0-63)
 * @tparam integer w rectangle width to grab
 * @tparam integer h rectangle height to grab
 */
int _screen_peek(lua_State *l) {
    lua_check_num_args(4);
    int x = luaL_checkinteger(l, 1);
    int y = luaL_checkinteger(l, 2);
    int w = luaL_checkinteger(l, 3);
    int h = luaL_checkinteger(l, 4);
    lua_settop(l, 0);
    if ((x >= 0) && (x <= 127)
     && (y >= 0) && (y <= 63)
     && (w > 0)
     && (h > 0)) {
        screen_event_peek(x, y, w, h);
        screen_results_wait();
        union screen_results_data *results = screen_results_get();
        lua_pushlstring(l, results->peek.buf, results->peek.w * results->peek.h);
        screen_results_free();
    } else { 
        fprintf(stderr, "WARNING: invalid position arguments to screen_peek()\n");
        lua_pushlstring(l, "", 0);
    }
    return 1;
}

/***
 * screen: poke
 * @function s_poke
 * @tparam integer x screen x position (0-127)
 * @tparam integer y screen y position (0-63)
 * @tparam integer w rectangle width to replace
 * @tparam integer h rectangle height to replace
 * @tparam string buf pixel contents to set
 */
int _screen_poke(lua_State *l) {
    lua_check_num_args(5);
    int x = luaL_checkinteger(l, 1);
    int y = luaL_checkinteger(l, 2);
    size_t w = luaL_checkinteger(l, 3);
    size_t h = luaL_checkinteger(l, 4);
    size_t len;
    uint8_t *buf = (uint8_t *)luaL_checklstring(l, 5, &len);
    lua_settop(l, 1);
    if (buf && len >= w * h) {
        if ((x >= 0) && (x <= 127)
         && (y >= 0) && (y <= 63)
         && (w > 0)
         && (h > 0)) {
            screen_event_poke(x, y, w, h, buf);
        }
    }
    lua_pop(l, 1);
    return 0;
}


/***
 * screen: rotate
 * @function s_rotate
 * @tparam float radians
 */
int _screen_rotate(lua_State *l) {
    lua_check_num_args(1);
    double r = luaL_checknumber(l, 1);
    screen_event_rotate(r);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: translate origin to new position
 * @function s_translate
 * @param x
 * @param y
 */
int _screen_translate(lua_State *l) {
    lua_check_num_args(2);
    double x = luaL_checknumber(l, 1);
    double y = luaL_checknumber(l, 2);
    screen_event_translate(x, y);
    lua_settop(l, 0);
    return 0;
}


/***
 * screen: set_operator
 * @function s_set_operator
 * @tparam int operator_type (0 < 23)
 */
int _screen_set_operator(lua_State *l) {
    lua_check_num_args(1);
    int i = luaL_checknumber(l, 1);
    if (i < 0) { i = 0; }
    if (i > 28){ i = 28;}
    screen_event_set_operator(i);
    lua_settop(l, 0);
    return 0;
}

// clang-format off
static luaL_Reg _image_methods[] = {
    {"__gc", _image_free},
    {"__tostring", _image_tostring},
    {"__eq", _image_equals},
    {"_context_focus", _image_context_focus},
    {"_context_defocus", _image_context_defocus},
    {"extents", _image_extents},
    {"name", _image_name},
    {NULL, NULL}
};

static luaL_Reg _image_functions[] = {
    {"screen_load_png", _screen_load_png},
    {"screen_create_image", _screen_create_image},
    {"screen_display_image", _screen_display_image},
    {"screen_display_image_region", _screen_display_image_region},
    {NULL, NULL}
};
// clang-format on

int _image_new(lua_State *l, screen_surface_t *surface, const char *name) {
    _image_t *ud = (_image_t *)lua_newuserdata(l, sizeof(_image_t));
    ud->surface = surface;
    ud->context = NULL;
    ud->previous_context = NULL;
    if (name != NULL) {
        ud->name = strdup(name);
    } else {
        ud->name = NULL;
    }
    luaL_getmetatable(l, _image_class_name);
    lua_setmetatable(l, -2);
    return 1;
}

int _image_context_focus(lua_State *l) {
    _image_t *i = _image_check(l, 1);
    if (i->context == NULL) {
        // lazily allocate drawing context
        i->context = screen_context_new(i->surface);
        if (i->context == NULL) {
            luaL_error(l, "unable to create drawing context");
        }
    }
    i->previous_context = screen_context_get_current();
    screen_context_set(i->context);
    lua_settop(l, 0);
    return 0;
}

static void __image_context_defocus(_image_t *i) {
    if (i->previous_context != NULL) {
        screen_context_set(i->previous_context);
        i->previous_context = NULL;
    } else {
        screen_context_set_primary();
    }
}

int _image_context_defocus(lua_State *l) {
    __image_context_defocus(_image_check(l, 1));
    lua_settop(l, 0);
    return 0;
}

_image_t *_image_check(lua_State *l, int arg) {
    void *ud = luaL_checkudata(l, arg, _image_class_name);
    luaL_argcheck(l, ud != NULL, arg, "image object expected");
    return (_image_t *)ud;
}

int _image_free(lua_State *l) {
    _image_t *i = _image_check(l, 1);
    // fprintf(stderr, "_image_free(%p): begin\n", i);
    screen_surface_free(i->surface);
    if (i->context != NULL) {
        if (i->context == screen_context_get_current()) {
            // automatically defocus ourselves if we are the current drawing target
            // fprintf(stderr, "_image_free(%p): auto defocus\n", i);
            __image_context_defocus(i);
        }
        screen_context_free(i->context);
    }
    if (i->name != NULL) {
        free(i->name);
    }
    // fprintf(stderr, "_image_free(%p): end\n", i);
    return 0;
}

int _image_tostring(lua_State *l) {
    char repr[255];
    _image_t *i = _image_check(l, 1);
    if (i->name != NULL) {
        snprintf(repr, sizeof(repr), "%s: %p (%s)", _image_class_name, i, i->name);
    } else {
        snprintf(repr, sizeof(repr), "%s: %p", _image_class_name, i);
    }
    lua_pushstring(l, repr);
    return 1;
}

int _image_equals(lua_State *l) {
    _image_t *a = _image_check(l, 2);
    _image_t *b = _image_check(l, 1);
    lua_pushboolean(l, a->surface == b->surface);
    return 1;
}

int _image_extents(lua_State *l) {
    screen_surface_extents_t extents;
    _image_t *i = _image_check(l, 1);
    if (screen_surface_get_extents(i->surface, &extents)) {
        lua_pushinteger(l, extents.width);
        lua_pushinteger(l, extents.height);
        return 2;
    }
    return 0;
}

int _image_name(lua_State *l) {
    _image_t *i = _image_check(l, 1);
    if (i->name) {
        lua_pushstring(l, i->name);
        return 1;
    }
    return 0;
}

/***
 * screen: create_image
 * @function screen_create_imate
 * @tparam number width image width
 * @tparam number height image height
 */
int _screen_create_image(lua_State *l) {
    lua_check_num_args(2);
    double width = luaL_checknumber(l, 1);
    double height = luaL_checknumber(l, 2);
    if (width < 1 || height < 1) {
        luaL_error(l, "image dimensions too small; must be >= 1");
    }
    screen_surface_t *s = screen_surface_new(width, height);
    if (s != NULL) {
        return _image_new(l, s, NULL);
    }
    luaL_error(l, "image creation failed");
    return 0;
}

/***
 * screen: load_png
 * @function screen_load_png
 * @tparam string string file path
 */
int _screen_load_png(lua_State *l) {
    lua_check_num_args(1);
    const char *name = luaL_checkstring(l, 1);
    screen_surface_t *s = screen_surface_load_png(name);
    if (s != NULL) {
        return _image_new(l, s, name);
    }
    luaL_error(l, "image loading failed");
    return 0;
}

/***
 * screen: display_image
 * @function screen_display_image
 * @tparam image image object
 * @tparam number x position
 * @tparam number y position
 */
int _screen_display_image(lua_State *l) {
    lua_check_num_args(3);
    _image_t *i = _image_check(l, 1);
    double x = luaL_checknumber(l, 2);
    double y = luaL_checknumber(l, 3);
    screen_event_display_surface(i->surface, x, y);
    lua_settop(l, 0);
    return 0;
}

/***
 * screen: display_image_region
 * @function screen_display_image_region
 * @tparam image image object
 * @tparam number left inset within image
 * @tparam number top inset within image
 * @tparam number width from right within image
 * @tparam number height from top within image
 * @tparam number x position
 * @tparam number y position
 */
int _screen_display_image_region(lua_State *l) {
    lua_check_num_args(7);
    _image_t *i = _image_check(l, 1);
    double left = luaL_checknumber(l, 2);
    double top = luaL_checknumber(l, 3);
    double width = luaL_checknumber(l, 4);
    double height = luaL_checknumber(l, 5);
    double x = luaL_checknumber(l, 6);
    double y = luaL_checknumber(l, 7);
    screen_event_display_surface_region(i->surface, left, top, width, height, x, y);
    return 0;
}


/***
 * screen: request current draw point
 */
int _screen_current_point(lua_State *l) {
    lua_settop(l, 0);
    screen_event_current_point();
    screen_results_wait();
    union screen_results_data *results = screen_results_get();
    lua_pushnumber(l, results->current_point.x);
    lua_pushnumber(l, results->current_point.y);
    screen_results_free();
    return 2;
}

///-- end screen commands
//---------------------




/***
 * headphone: set level
 * @function gain_hp
 * @tparam integer level level (0-63)
 */
int _gain_hp(lua_State *l) {
    lua_check_num_args(1);
    int level = (int)luaL_checkinteger(l, 1);
    i2c_hp(level);
    lua_settop(l, 0);
    return 0;
}

int _adc_rev(lua_State *l) {
    int rev = adc_rev();
    lua_pushinteger(l, (lua_Integer)rev);
    return 1;
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

    if ((host == NULL) || (port == NULL) || (path == NULL)) {
        return 1;
    }

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
                luaL_error(l, "invalid osc argument type %s", lua_typename(l, argtype));
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

    if (path == NULL) {
        return 1;
    }

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
                luaL_error(l, "invalid osc argument type %s", lua_typename(l, argtype));
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

int _serial_send(lua_State *l) {
    struct dev_serial *d;
    const char *s;

    if (lua_gettop(l) != 2) {
        return luaL_error(l, "wrong number of arguments");
    }

    luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
    d = lua_touserdata(l, 1);
    size_t len = 0;
    s = luaL_checklstring(l, 2, &len);
    lua_settop(l, 0);

    dev_serial_send(d, s, len);

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
 * midi: clock_receive
 * @function midi_receive
 */
int _midi_clock_receive(lua_State *l) {
    struct dev_midi *md;
    lua_check_num_args(2);
    luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
    md = lua_touserdata(l, 1);
    int enabled = lua_tointeger(l, 2);
    md->clock_enabled = enabled > 0;
    //fprintf(stderr, "set clock_enabled to %d on device %p\n", enabled, md);
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
    int x = (int)luaL_checkinteger(l, 2) - 1; // convert from 1-base
    int y = (int)luaL_checkinteger(l, 3) - 1; // convert from 1-base
    int z = (int)luaL_checkinteger(l, 4);     // don't convert value!
    dev_monome_grid_set_led(md, x, y, z);
    lua_settop(l, 0);
    return 0;
}

int _arc_set_led(lua_State *l) {
    lua_check_num_args(4);
    luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
    struct dev_monome *md = lua_touserdata(l, 1);
    int n = (int)luaL_checkinteger(l, 2) - 1; // convert from 1-base
    int x = (int)luaL_checkinteger(l, 3) - 1; // convert from 1-base
    int val = (int)luaL_checkinteger(l, 4);   // don't convert value!
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
    int z = (int)luaL_checkinteger(l, 2); // don't convert value!
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
    int z = (int)luaL_checkinteger(l, 2); // don't convert value!
    dev_monome_set_rotation(md, z);
    lua_settop(l, 0);
    return 0;
}

/***
 * grid: enable tilt
 * @param dev grid device
 * @param id sensor number
 */
int _grid_tilt_enable(lua_State *l) {
    lua_check_num_args(2);
    luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
    struct dev_monome *md = lua_touserdata(l, 1);
    int id = (int)luaL_checkinteger(l, 2); // don't convert value!
    dev_monome_tilt_enable(md, id);
    lua_settop(l, 0);
    return 0;
}
/***
 * grid: disable tilt
 * @param dev grid device
 * @param id sensor number
 */
int _grid_tilt_disable(lua_State *l) {
    lua_check_num_args(2);
    luaL_checktype(l, 1, LUA_TLIGHTUSERDATA);
    struct dev_monome *md = lua_touserdata(l, 1);
    int id = (int)luaL_checkinteger(l, 2); // don't convert value!
    dev_monome_tilt_disable(md, id);
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
    int i = (int)luaL_checkinteger(l, 2); // don't convert value!
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

    int idx = (int)luaL_checkinteger(l, 1) - 1; // 1-base to 0-base
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

    for (int i = 2; i <= nargs; i++) {
        switch (fmt[i - 2]) {
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
                d = (int)lua_tonumber(l, i);
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
    int count = -1;        // default: infinite
    int stage = 0;

    int nargs = lua_gettop(l);

    if (nargs > 0) {
        idx = (int)luaL_checkinteger(l, 1) - 1; // convert from 1-based
    }

    if (nargs > 1) {
        seconds = (double)luaL_checknumber(l, 2);
    }

    if (nargs > 2) {
        count = (int)luaL_checkinteger(l, 3);
    }

    if (nargs > 3) {
        stage = (int)luaL_checkinteger(l, 4) - 1; // convert from 1-based
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
    int idx = (int)luaL_checkinteger(l, 1) - 1;
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
    int idx = (int)luaL_checkinteger(l, 1) - 1;
    float sec = (float)luaL_checknumber(l, 2);
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
    int usec = (float)luaL_checknumber(l, 1);
    usleep(usec);
    lua_settop(l, 0);
    return 0;
}

//---- c -> lua glue

//--- hardware input:

// helper for calling grid handlers
static inline void _call_grid_handler(int id, int x, int y, int state) {
    _push_norns_func("grid", "key");
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    lua_pushinteger(lvm, x + 1);  // convert to 1-base
    lua_pushinteger(lvm, y + 1);  // convert to 1-base
    lua_pushinteger(lvm, state);
    l_report(lvm, l_docall(lvm, 4, 0));
}

int _clock_schedule_sleep(lua_State *l) {
    lua_check_num_args(2);
    int coro_id = (int)luaL_checkinteger(l, 1);
    double seconds = luaL_checknumber(l, 2);

    if (seconds < 0) {
        seconds = 0;
    }

    clock_scheduler_schedule_sleep(coro_id, seconds);

    return 0;
}

int _clock_schedule_sync(lua_State *l) {
    int coro_id = (int)luaL_checkinteger(l, 1);
    double sync_beat = luaL_checknumber(l, 2);
    double offset = luaL_optnumber(l, 3, 0);

    if (sync_beat <= 0) {
        luaL_error(l, "invalid sync beat: %f", sync_beat);
    } else {
        clock_scheduler_schedule_sync(coro_id, sync_beat, offset);
    }

  return 0;
}

int _clock_cancel(lua_State *l) {
    lua_check_num_args(1);
    int coro_id = (int)luaL_checkinteger(l, 1);
    clock_scheduler_clear(coro_id);
    return 0;
}

int _clock_internal_set_tempo(lua_State *l) {
    lua_check_num_args(1);
    double bpm = luaL_checknumber(l, 1);
    clock_internal_set_tempo(bpm);
    return 0;
}

int _clock_internal_start(lua_State *l) {
    clock_internal_restart();
    return 0;
}

int _clock_internal_stop(lua_State *l) {
    clock_internal_stop();
    return 0;
}

int _clock_crow_in_div(lua_State *l) {
    lua_check_num_args(1);
    int div = (int)luaL_checkinteger(l, 1);
    clock_crow_in_div(div);
    return 0;
}

#if HAVE_ABLETON_LINK
int _clock_link_get_number_of_peers(lua_State *l) {
    uint64_t peers = clock_link_number_of_peers();
    lua_pushinteger(l, (lua_Integer)peers);
    return 1;
}

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


int _clock_link_set_transport_start(lua_State *l) {
    clock_link_set_transport_start();
    return 0;
}

int _clock_link_set_transport_stop(lua_State *l) {
    clock_link_set_transport_stop();
    return 0;
}

int _clock_link_set_start_stop_sync(lua_State *l) {
    lua_check_num_args(1);
    bool enabled = lua_toboolean(l, 1);
    clock_link_set_start_stop_sync(enabled);
    return 0;
}
#endif

int _clock_set_source(lua_State *l) {
    lua_check_num_args(1);
    int source = (int)luaL_checkinteger(l, 1);
    clock_set_source(source);
    return 0;
}

int _clock_get_time_beats(lua_State *l) {
    lua_pushnumber(l, clock_get_beats());
    return 1;
}

int _clock_get_tempo(lua_State *l) {
    lua_pushnumber(l, clock_get_tempo());
    return 1;
}

int _audio_get_cpu_load(lua_State *l) {
    lua_pushnumber(l, jack_client_get_cpu_load());
    return 1;
}

int _audio_get_xrun_count(lua_State *l) {
    lua_pushnumber(l, jack_client_get_xrun_count());
    return 1;
}

int _cpu_time_start_timer(lua_State *l) {
    cpu_time_start();
    return 0;
}

int _cpu_time_get_delta(lua_State *l) {
    lua_pushnumber(l, cpu_time_get_delta_ns());
    return 1;
}

int _wall_time_start_timer(lua_State *l) {
    wall_time_start();
    return 0;
}

int _wall_time_get_delta(lua_State *l) {
    lua_pushnumber(l, wall_time_get_delta_ns());
    return 1;
}

//--------------------------------------------------
//--- define lua handlers for system callbacks

void w_handle_monome_add(void *mdev) {
    struct dev_monome *md = (struct dev_monome *)mdev;
    int id = md->dev.id;
    const char *serial = md->dev.serial;
    const char *name = md->dev.name;
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

void w_handle_grid_tilt(int id, int sensor, int x, int y, int z) {
    _push_norns_func("grid", "tilt");
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    lua_pushinteger(lvm, sensor + 1);  // convert to 1-base
    lua_pushinteger(lvm, x + 1);  // convert to 1-base
    lua_pushinteger(lvm, y + 1);  // convert to 1-base
    lua_pushinteger(lvm, z + 1);
    l_report(lvm, l_docall(lvm, 5, 0));
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
    for (int i = 0; i < ntypes; i++) {
        lua_pushinteger(lvm, dev->types[i]);
        lua_rawseti(lvm, -2, i + 1);
    }

    // table of tables of event codes
    lua_createtable(lvm, ntypes, 0);
    for (int i = 0; i < ntypes; i++) {
        int ncodes = dev->num_codes[i];
        lua_createtable(lvm, ncodes, 0);
        for (int j = 0; j < ncodes; j++) {
            lua_pushinteger(lvm, dev->codes[i][j]);
            lua_rawseti(lvm, -2, j + 1);
        }
        lua_rawseti(lvm, -2, i + 1);
    }

    lua_pushlightuserdata(lvm, dev);
    lua_pushstring(lvm, dev->guid);
    l_report(lvm, l_docall(lvm, 6, 0));
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

void w_handle_serial_config(char *path, char *name, char *vendor, char *model, char *serial, char *interface) {
    _push_norns_func("serial", "match");
    lua_pushstring(lvm, vendor);
    lua_pushstring(lvm, model);
    lua_pushstring(lvm, serial);
    lua_pushstring(lvm, interface);
    l_report(lvm, l_docall(lvm, 4, 1));
    if (lua_isnil(lvm, -1)) {
        fprintf(stderr, "no serial handler found for device %s at %s\n", name, path);
        return;
    }
    if (!lua_isstring(lvm, -1)) {
        fprintf(stderr, "serial handler id expected, got %s\n", lua_typename(lvm, lua_type(lvm, -1)));
        return;
    }

    dev_list_add(DEV_TYPE_SERIAL, path, strdup(name), lvm);
}


void w_handle_serial_add(void *p) {
    struct dev_serial *dev = (struct dev_serial *)p;
    struct dev_common *base = (struct dev_common *)p;
    int id = base->id;

    _push_norns_func("serial", "add");
    lua_pushstring(lvm, dev->handler_id);
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    lua_pushstring(lvm, base->name);
    lua_pushlightuserdata(lvm, dev);
    l_report(lvm, l_docall(lvm, 4, 0));
}

void w_handle_serial_remove(uint32_t id, char *handler_id) {
    _push_norns_func("serial", "remove");
    lua_pushstring(lvm, handler_id);
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    l_report(lvm, l_docall(lvm, 2, 0));
}

void w_handle_serial_event(void *dev, uint32_t id, char *data, ssize_t len) {
    struct dev_serial *d = (struct dev_serial *)dev;
    _push_norns_func("serial", "event");
    lua_pushstring(lvm, d->handler_id);
    lua_pushinteger(lvm, id + 1); // convert to 1-base
    lua_pushlstring(lvm, data, len);
    l_report(lvm, l_docall(lvm, 3, 0));
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

void w_handle_osc_event(char *from_host, char *from_port, char *path, lo_message msg) {
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
            lua_pushlstring(lvm, lo_blob_dataptr((lo_blob)argv[i]), lo_blob_datasize((lo_blob)argv[i]));
            break;
        case LO_INT64:
            lua_pushinteger(lvm, argv[i]->h);
            break;
        case LO_DOUBLE:
            lua_pushnumber(lvm, argv[i]->d);
            break;
        case LO_SYMBOL:
            lua_pushstring(lvm, &argv[i]->S);
            break;
        case LO_CHAR:
            lua_pushlstring(lvm, (const char *)&argv[i]->c, 1);
            break;
        case LO_MIDI:
            lua_pushlstring(lvm, (const char *)&argv[i]->m, 4);
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
static inline void _push_string_array(const char **arr, const int n) {
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
    for (int i = 0; i < n; i++) {
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
    for (int i = 0; i < n; ++i) {
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

// clock handlers
void w_handle_clock_resume(const int coro_id, double value) {
    lua_getglobal(lvm, "clock");
    lua_getfield(lvm, -1, "resume");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, coro_id);
    lua_pushnumber(lvm, value);
    l_report(lvm, l_docall(lvm, 2, 0));
}

void w_handle_clock_start() {
    _push_norns_func("clock", "start");
    l_report(lvm, l_docall(lvm, 0, 0));
}

void w_handle_clock_stop() {
    _push_norns_func("clock", "stop");
    l_report(lvm, l_docall(lvm, 0, 0));
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
void w_handle_stat(const uint32_t disk, const uint16_t temp, const uint16_t cpu,
    const uint16_t cpu1, const uint16_t cpu2, const uint16_t cpu3, const uint16_t cpu4) {
    lua_getglobal(lvm, "_norns");
    lua_getfield(lvm, -1, "stat");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, disk);
    lua_pushinteger(lvm, temp);
    lua_pushinteger(lvm, cpu);
    lua_pushinteger(lvm, cpu1);
    lua_pushinteger(lvm, cpu2);
    lua_pushinteger(lvm, cpu3);
    lua_pushinteger(lvm, cpu4);
    l_report(lvm, l_docall(lvm, 7, 0));
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
    for (int i = 0; i < size; ++i) {
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
    for (int i = 0; i < 4; ++i) {
        lua_pushinteger(lvm, levels[i]);
    }
    l_report(lvm, l_docall(lvm, 4, 0));
}

void w_handle_poll_softcut_phase(int idx, float val) {
    // fprintf(stderr, "_handle_poll_softcut_phase: %d, %f\n", idx, val);
    lua_getglobal(lvm, "_norns");
    lua_getfield(lvm, -1, "softcut_phase");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, idx + 1);
    lua_pushnumber(lvm, val);
    l_report(lvm, l_docall(lvm, 2, 0));
}

void w_handle_softcut_render(int idx, float sec_per_sample, float start, size_t size, float* data) {
    lua_getglobal(lvm, "_norns");
    lua_getfield(lvm, -1, "softcut_render");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, idx + 1);
    lua_pushnumber(lvm, start);
    lua_pushnumber(lvm, sec_per_sample);
    lua_createtable(lvm, size, 0);
    for (size_t i = 0; i < size; ++i) {
        lua_pushnumber(lvm, data[i]);
        lua_rawseti(lvm, -2, i + 1);
    }
    l_report(lvm, l_docall(lvm, 4, 0));
}

void w_handle_softcut_position(int idx, float pos) {
    lua_getglobal(lvm, "_norns");
    lua_getfield(lvm, -1, "softcut_position");
    lua_remove(lvm, -2);
    lua_pushinteger(lvm, idx + 1);
    lua_pushnumber(lvm, pos);
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

void w_handle_screen_refresh() {
    lua_getglobal(lvm, "refresh");
    l_report(lvm, l_docall(lvm, 0, 0));
}

void w_handle_custom_weave(struct event_custom *ev) {
    // call the externally defined `op` function passing in the current lua
    // state
    ev->ops->weave(lvm, ev->value, ev->context);
}

// helper: set poll given by lua to given state
static int poll_set_state(lua_State *l, bool val) {
    lua_check_num_args(1);
    int idx = (int)luaL_checkinteger(l, 1) - 1; // convert from 1-based
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
     int idx = (int)luaL_checkinteger(l, 1) - 1; // convert from 1-based
    float val = (float)luaL_checknumber(l, 2);
    o_set_poll_time(idx, val);
    lua_settop(l, 0);
    return 0;
}

int _request_poll_value(lua_State *l) {
    lua_check_num_args(1);
    int idx = (int)luaL_checkinteger(l, 1) - 1; // convert from 1-based
    o_request_poll_value(idx);
    lua_settop(l, 0);
    return 0;
}

// audio context control
int _set_level_adc(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_adc(val);
    lua_settop(l, 0);
    return 0;
}

int _set_level_dac(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_dac(val);
    lua_settop(l, 0);
    return 0;
}

int _set_level_ext(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_ext(val);
    lua_settop(l, 0);
    return 0;
}

int _set_level_monitor(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
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
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_tape(val);
    lua_settop(l, 0);
    return 0;
}

int _tape_rec_open(lua_State *l) {
    lua_check_num_args(1);
    const char *s = luaL_checkstring(l, 1);
    o_tape_rec_open((char *)s);
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
    o_tape_play_open((char *)s);
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
    int idx = (int)luaL_checkinteger(l, 1) - 1;
    float val = (float)luaL_checknumber(l, 2);
    o_cut_enable(idx, val);
    return 0;
}

int _set_level_adc_cut(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_adc_cut(val);
    return 0;
}

int _set_level_ext_cut(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_ext_cut(val);
    return 0;
}

int _set_level_tape_cut(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_tape_cut(val);
    return 0;
}

int _set_level_cut_rev(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_cut_rev(val);
    return 0;
}

int _set_level_cut_master(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_cut_master(val);
    return 0;
}

int _set_level_cut(lua_State *l) {
    lua_check_num_args(2);
    int idx = (int)luaL_checkinteger(l, 1) - 1;
    float val = (float)luaL_checknumber(l, 2);
    o_set_level_cut(idx, val);
    return 0;
}

int _set_level_cut_cut(lua_State *l) {
    lua_check_num_args(3);
    int src = (int)luaL_checkinteger(l, 1) - 1;
    int dest = (int)luaL_checkinteger(l, 2) - 1;
    float val = (float)luaL_checknumber(l, 3);
    o_set_level_cut_cut(src, dest, val);
    return 0;
}

int _set_pan_cut(lua_State *l) {
    lua_check_num_args(2);
    int idx = (int)luaL_checkinteger(l, 1) - 1;
    float val = (float)luaL_checknumber(l, 2);
    o_set_pan_cut(idx, val);
    return 0;
}

int _cut_buffer_clear(lua_State *l) {
    o_cut_buffer_clear();
    return 0;
}

int _cut_buffer_clear_channel(lua_State *l) {
    lua_check_num_args(1);
    int ch = (int)luaL_checkinteger(l, 1) - 1;
    o_cut_buffer_clear_channel(ch);
    return 0;
}

int _cut_buffer_clear_region(lua_State *l) {
    lua_check_num_args(4);
    float start = (float)luaL_checknumber(l, 1);
    float dur = (float)luaL_checknumber(l, 2);
    float fade = (float)luaL_checknumber(l, 3);
    float preserve = (float)luaL_checknumber(l, 4);
    o_cut_buffer_clear_region(start, dur, fade, preserve);
    return 0;
}

int _cut_buffer_clear_region_channel(lua_State *l) {
    lua_check_num_args(5);
    int ch = (int)luaL_checkinteger(l, 1) - 1;
    float start = (float)luaL_checknumber(l, 2);
    float dur = (float)luaL_checknumber(l, 3);
    float fade_time = (float)luaL_checknumber(l, 4);
    float preserve = (float)luaL_checknumber(l, 5);
    o_cut_buffer_clear_region_channel(ch, start, dur, fade_time, preserve);
    return 0;
}

int _cut_buffer_copy_mono(lua_State *l) {
    lua_check_num_args(8);
    int src_ch = (int)luaL_checkinteger(l, 1) - 1;
    int dst_ch = (int)luaL_checkinteger(l, 2) - 1;
    float src_start = (float)luaL_checknumber(l, 3);
    float dst_start = (float)luaL_checknumber(l, 4);
    float dur = (float)luaL_checknumber(l, 5);
    float fade_time = (float)luaL_checknumber(l, 6);
    float preserve = (float)luaL_checknumber(l, 7);
    int reverse = (int)luaL_checkinteger(l, 8);
    o_cut_buffer_copy_mono(src_ch, dst_ch, src_start, dst_start, dur, fade_time, preserve, reverse);
    return 0;
}

int _cut_buffer_copy_stereo(lua_State *l) {
    lua_check_num_args(6);
    float srcStart = (float)luaL_checknumber(l, 1);
    float dstStart = (float)luaL_checknumber(l, 2);
    float dur = (float)luaL_checknumber(l, 3);
    float fadeTime = (float)luaL_checknumber(l, 4);
    float preserve = (float)luaL_checknumber(l, 5);
    int reverse = (int)luaL_checkinteger(l, 6);
    o_cut_buffer_copy_stereo(srcStart, dstStart, dur, fadeTime, preserve, reverse);
    return 0;
}

int _cut_buffer_read_mono(lua_State *l) {
    lua_check_num_args(8);
    const char *s = luaL_checkstring(l, 1);
    float start_src = (float)luaL_checknumber(l, 2);
    float start_dst = (float)luaL_checknumber(l, 3);
    float dur = (float)luaL_checknumber(l, 4);
    int ch_src = (int)luaL_checkinteger(l, 5) - 1;
    int ch_dst = (int)luaL_checkinteger(l, 6) - 1;
    float preserve = (float)luaL_checknumber(l, 7);
    float mix = (float)luaL_checknumber(l, 8);
    o_cut_buffer_read_mono((char *)s, start_src, start_dst, dur, ch_src, ch_dst, preserve, mix);
    return 0;
}

int _cut_buffer_read_stereo(lua_State *l) {
    lua_check_num_args(6);
    const char *s = luaL_checkstring(l, 1);
    float start_src = (float)luaL_checknumber(l, 2);
    float start_dst = (float)luaL_checknumber(l, 3);
    float dur = (float)luaL_checknumber(l, 4);
    float preserve = (float)luaL_checknumber(l, 5);
    float mix = (float)luaL_checknumber(l, 6);
    o_cut_buffer_read_stereo((char *)s, start_src, start_dst, dur, preserve, mix);
    return 0;
}

int _cut_buffer_write_mono(lua_State *l) {
    lua_check_num_args(4);
    const char *s = luaL_checkstring(l, 1);
    float start = (float)luaL_checknumber(l, 2);
    float dur = (float)luaL_checknumber(l, 3);
    int ch = (int)luaL_checkinteger(l, 4) - 1;
    o_cut_buffer_write_mono((char *)s, start, dur, ch);
    return 0;
}

int _cut_buffer_write_stereo(lua_State *l) {
    lua_check_num_args(3);
    const char *s = luaL_checkstring(l, 1);
    float start = (float)luaL_checknumber(l, 2);
    float dur = (float)luaL_checknumber(l, 3);
    o_cut_buffer_write_stereo((char *)s, start, dur);
    return 0;
}

int _cut_buffer_render(lua_State *l) {
    lua_check_num_args(4);
    int ch = (int)luaL_checkinteger(l, 1) - 1;
    float start = (float)luaL_checknumber(l, 2);
    float dur = (float)luaL_checknumber(l, 3);
    int samples = (int)luaL_checknumber(l, 4);
    o_cut_buffer_render(ch, start, dur, samples);
    return 0;
}

int _cut_query_position(lua_State *l) {
    lua_check_num_args(1);
    int i = (int)luaL_checkinteger(l, 1) - 1;
    o_cut_query_position(i);
    return 0;
}

int _cut_reset(lua_State *l) {
    o_cut_reset();
    return 0;
}

int _set_cut_param(lua_State *l) {
    lua_check_num_args(3);
    const char *s = luaL_checkstring(l, 1);
    int voice = (int)luaL_checkinteger(l, 2) - 1;
    float val = (float)luaL_checknumber(l, 3);
    o_set_cut_param((char *)s, voice, val);
    return 0;
}

int _set_cut_param_ii(lua_State *l) {
    lua_check_num_args(3);
    const char *s = luaL_checkstring(l, 1);
    int a = (int)luaL_checkinteger(l, 2) - 1;
    int b = (int)luaL_checkinteger(l, 3) - 1;
    o_set_cut_param_ii((char *)s, a, b);
    return 0;
}

int _set_cut_param_iif(lua_State *l) {
    lua_check_num_args(4);
    const char *s = luaL_checkstring(l, 1);
    int a = (int)luaL_checkinteger(l, 2) - 1;
    int b = (int)luaL_checkinteger(l, 3) - 1;
    float val = (float)luaL_checknumber(l, 4);
    o_set_cut_param_iif((char *)s, a, b, val);
    return 0;
}

int _set_level_input_cut(lua_State *l) {
    lua_check_num_args(3);
    int ch = (int)luaL_checkinteger(l, 1) - 1;
    int voice = (int)luaL_checkinteger(l, 2) - 1;
    float val = (float)luaL_checknumber(l, 3);
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
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_monitor_rev(val);
    return 0;
}

int _set_level_ext_rev(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_ext_rev(val);
    return 0;
}

int _set_level_tape_rev(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_tape_rev(val);
    return 0;
}

int _set_level_rev_dac(lua_State *l) {
    lua_check_num_args(1);
    float val = (float)luaL_checknumber(l, 1);
    o_set_level_rev_dac(val);
    return 0;
}

int _set_rev_param(lua_State *l) {
    lua_check_num_args(2);
    const char *s = luaL_checkstring(l, 1);
    float val = (float)luaL_checknumber(l, 2);
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
    float val = (float)luaL_checknumber(l, 1);
    o_set_comp_mix(val);
    return 0;
}

int _set_comp_param(lua_State *l) {
    lua_check_num_args(2);
    const char *s = luaL_checkstring(l, 1);
    float val = (float)luaL_checknumber(l, 2);
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

int _system_cmd(lua_State *l) {
    lua_check_num_args(1);
    const char *cmd = luaL_checkstring(l, 1);
    system_cmd((char *)cmd);
    return 0;
}

int _system_glob(lua_State *l) {
    lua_check_num_args(1);
    const char *pattern = luaL_checkstring(l, 1);

    int glob_status = 0;
    int glob_flags = GLOB_MARK | GLOB_TILDE | GLOB_BRACE;
    glob_t g;

    unsigned int i;

    glob_status = glob(pattern, glob_flags, NULL, &g);

    if (glob_status != 0) {
        lua_pushnil(l);
        lua_pushinteger(l, glob_status);
        return 2;
    }

    lua_newtable(l);
    for(i=1; i<=g.gl_pathc; i++) {
        lua_pushstring(l, g.gl_pathv[i-1]);
        lua_rawseti(l, -2, i);
    }
    globfree(&g);
    return 1;
}

int _platform(lua_State *l) {
    lua_check_num_args(0);
    lua_pushinteger(l, platform());
    return 1;
}


#pragma GCC diagnostic pop
