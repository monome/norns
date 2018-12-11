#pragma once
#include <stdarg.h>
#include <lo/lo.h>
#include <stdbool.h>
/*
 * oracle.h
 *
 * this module speaks with the Crone.
 *
 * user should not care about the method (IPC or otherwise.)
 *
 */

#include "lo/lo.h"

// enumerate types of polls
typedef enum {
    POLL_TYPE_VALUE, // returns a single float value
    POLL_TYPE_DATA   // returns an arbitrarily-size binary data blob
} poll_type_t;

// data structure for engine command descriptor/header
struct engine_command {
    char *name;   // name string
    char *format; // format string
};

// data structure for engine poll descriptor/headerx
struct engine_poll {
    char *name;       // name string
    poll_type_t type; // value or data
};

// data structure for engine parameters
struct engine_param {
    char *name;
    int busIdx; // control bus index
};



// initialize
extern void o_init();
// shutdown
extern void o_deinit();

// send query for audio engine boot completion
extern void o_query_startup();

//----------------------
//--- access param and buffer descriptors
// get count of desctipors
extern int o_get_num_engines(void);
extern int o_get_num_commands(void);
extern int o_get_num_polls(void);
extern int o_get_num_params(void);

// get engine names (array of c strings)
extern const char **o_get_engine_names(void);
// get available engine commands (array of structs)
extern const struct engine_command *o_get_commands(void);
// get available polls
extern const struct engine_poll *o_get_polls(void);

// lock/unlock the descriptor critical section
// FIXME: not ideal, but any use of the accessors above
// must be bracketed by these
extern void o_lock_descriptors();
extern void o_unlock_descriptors();

//-----------------------
//--- IPC functions

// load a named audio processing engine
extern void o_load_engine(const char *name);
// free engine
extern void o_free_engine(void);

// request list of engine names
extern void o_request_engine_report(void);

// request list of commands
extern void o_request_command_report(void);

// request list of polls
extern void o_request_poll_report(void);

// issue a command to the engine, adds /command/ pattern
// caller is responsible for freeing memory
extern void o_send_command(const char *name, lo_message msg);

// start or stop a poll
//extern void o_set_poll_state(const char *name, bool state);
extern void o_set_poll_state(int idx, bool state);

// set poll period
extern void o_set_poll_time(int idx, float dt);

// request current value of poll
extern void o_request_poll_value(int idx);

//--- audio context controls

extern void o_poll_start_vu();
extern void o_poll_stop_vu();

extern void o_set_audio_input_level(int idx, float level);
extern void o_set_audio_output_level(float level);
extern void o_set_level_ext(float level);
extern void o_set_audio_monitor_level(float level);
extern void o_set_audio_monitor_mono();
extern void o_set_audio_monitor_stereo();
extern void o_set_audio_monitor_on();
extern void o_set_audio_monitor_off();
extern void o_set_audio_pitch_on();
extern void o_set_audio_pitch_off();

//--- tape control
extern void o_level_tape(float level);
extern void o_tape_rec_open(char *file);
extern void o_tape_rec_start();
extern void o_tape_rec_stop();
extern void o_tape_play_open(char *file);
extern void o_tape_play_start();
extern void o_tape_play_stop();

//--- cut
extern void o_enable_cut(int i, float value);
extern void o_set_level_adc_cut(float value);
extern void o_set_level_ext_cut(float value);
extern void o_set_level_cut_aux(float value);
extern void o_set_level_cut(int index, float value);
extern void o_set_level_cut_cut(int src, int dest, float value);
extern void o_set_pan_cut(int index, float value);
extern void o_cut_buffer_clear_region(float start, float end);
extern void o_cut_buffer_clear();
extern void o_cut_buffer_read(char *file, float start_src,
                float start_dst, float dur, int ch);


//--- aux effects controls
// enable / disable aux fx processing
extern void o_set_aux_fx_on();
extern void o_set_aux_fx_off();
// mono input -> aux level
extern void o_set_aux_fx_input_level(int channel, float value);
// mono input -> aux pan
extern void o_set_aux_fx_input_pan(int channel, float value);
// stereo output -> aux
extern void o_set_aux_fx_output_level(float value);
// aux return -> dac
extern void o_set_aux_fx_return_level(float value);
extern void o_set_aux_fx_param(const char* name, float value);

//--- insert effects controls
extern void o_set_insert_fx_on();
extern void o_set_insert_fx_off();
extern void o_set_insert_fx_mix(float level);
extern void o_set_insert_fx_param(const char* name, float value);

//--- softcut parameter controls
//--- TODO: no lua glue for these yet


// most softcut parameter changs take single voice index...
extern void o_set_softcut_voice_param(const char* name, int voice, float value);
//... these ones take two channel indices
extern void o_set_softcut_input_level(int src, int dst, float level);
extern void o_set_softcut_feedback_level(int src, int dst, float level);



extern void o_restart_audio();
