/*
 * oracle.c
 *
 * implements communication with audio server for C programs.
 *
 * user should not care about the method (IPC or otherwise.)
 *
 * for now, we will use OSC with liblo.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "lo/lo.h"

#include "args.h"
#include "events.h"
#include "hello.h"
#include "oracle.h"

// address of external DSP environment (e.g. supercollider)
static lo_address ext_addr;
// address of crone process
static lo_address crone_addr;

static lo_server_thread st;

// TODO: semaphore for waiting on audio backend init?
//static sem_t audio_init_sem;

//-------------------
//--- audio engine descriptor management

// audio engine ready
int ready = 0;
// count of audio engine descriptors
int num_engines = 0;
// count of command descriptors
int num_commands = 0;
// count of poll descriptors
int num_polls = 0;

// state flags for receiving command/poll/param reports
bool needCommandReport;
bool needPollReport;
bool needParamReport = false; // FIXME

// max count of any single desciptor type
#define MAX_NUM_DESC 1024

// list of registered engine names
char *engine_names[MAX_NUM_DESC];
// list of registered engine commands
struct engine_command commands[MAX_NUM_DESC];
// list of registered poll
struct engine_poll polls[MAX_NUM_DESC];

// mutex for desctiptor data
pthread_mutex_t desc_lock;

//---------------------------------
//--- static functions

// intialize descriptor lists
static void o_init_descriptors(void);
// clear a given array of descriptors
void o_clear_engine_names(void);
void o_clear_commands(void);
void o_clear_polls(void);

// set a given entry in the engine name list
static void o_set_engine_name(int idx, const char *name);
static void o_set_command(int idx, const char *name, const char *format);
// set a given descriptor count variable
static void o_set_num_desc(int *dst, int num);

//--- OSC handlers

static int handle_crone_ready(const char *path, const char *types,
                              lo_arg **argv, int argc,
                              void *data, void *user_data);
static int handle_engine_report_start(const char *path, const char *types,
                                      lo_arg **argv, int argc,
                                      void *data, void *user_data);
static int handle_engine_report_entry(const char *path, const char *types,
                                      lo_arg **argv, int argc,
                                      void *data, void *user_data);
static int handle_engine_report_end(const char *path, const char *types,
                                    lo_arg **argv, int argc,
                                    void *data, void *user_data);
static int handle_command_report_start(const char *path, const char *types,
                                       lo_arg **argv, int argc,
                                       void *data, void *user_data);
static int handle_command_report_entry(const char *path, const char *types,
                                       lo_arg **argv, int argc,
                                       void *data, void *user_data);
static int handle_command_report_end(const char *path, const char *types,
                                     lo_arg **argv, int argc,
                                     void *data, void *user_data);
static int handle_poll_report_start(const char *path, const char *types,
                                    lo_arg **argv, int argc,
                                    void *data, void *user_data);
static int handle_poll_report_entry(const char *path, const char *types,
                                    lo_arg **argv, int argc,
                                    void *data, void *user_data);
static int handle_poll_report_end(const char *path, const char *types,
                                  lo_arg **argv, int argc,
                                  void *data, void *user_data);
static int handle_poll_value(const char *path, const char *types,
                             lo_arg **argv, int argc,
                             void *data, void *user_data);
static int handle_poll_data(const char *path, const char *types,
                            lo_arg **argv, int argc,
                            void *data, void *user_data);
/* static int handle_poll_wave(const char *path, const char *types, */
/*                              lo_arg **argv, int argc, */
/*                              void *data, void *user_data); */
static int handle_poll_io_levels(const char *path, const char *types,
                                 lo_arg **argv, int argc,
                                 void *data, void *user_data);

static int handle_tape_play_state(const char *path, const char *types,
                                 lo_arg **argv, int argc,
                                 void *data, void *user_data);

static void lo_error_handler(int num, const char *m, const char *path);

static void set_need_reports() {
    needCommandReport = true;
    needPollReport = true;
    needParamReport = false; // FIXME true;
}

static bool get_need_reports() {
    return needCommandReport || needPollReport || needParamReport;
}

static void test_engine_load_done();

//-----------------------------------
//---- extern function definitions

void o_query_startup(void) {
    // fprintf(stderr, "sending /ready: %d", rem_port);
    lo_send(ext_addr, "/ready","");
}

//--- init
void o_init(void) {
    const char *local_port = args_local_port();
    const char *ext_port = args_ext_port();
        const char *crone_port = args_crone_port();

    fprintf(stderr, "OSC rx port: %s \nOSC tx port: %s\n",
            local_port, ext_port);
    o_init_descriptors();

    ext_addr = lo_address_new("127.0.0.1", ext_port);
    crone_addr = lo_address_new("127.0.0.1", crone_port);
    st = lo_server_thread_new(local_port, lo_error_handler);

    // crone ready
    lo_server_thread_add_method(st, "/crone/ready", "",
                                handle_crone_ready, NULL);
    // engine report sequence
    lo_server_thread_add_method(st, "/report/engines/start", "i",
                                handle_engine_report_start, NULL);
    lo_server_thread_add_method(st, "/report/engines/entry", "is",
                                handle_engine_report_entry, NULL);
    lo_server_thread_add_method(st, "/report/engines/end", "",
                                handle_engine_report_end, NULL);

    // command report sequence
    lo_server_thread_add_method(st, "/report/commands/start", "i",
                                handle_command_report_start, NULL);
    lo_server_thread_add_method(st, "/report/commands/entry", "iss",
                                handle_command_report_entry, NULL);
    lo_server_thread_add_method(st, "/report/commands/end", "",
                                handle_command_report_end, NULL);

    // poll report sequence
    lo_server_thread_add_method(st, "/report/polls/start", "i",
                                handle_poll_report_start, NULL);
    lo_server_thread_add_method(st, "/report/polls/entry", "isi",
                                handle_poll_report_entry, NULL);
    lo_server_thread_add_method(st, "/report/polls/end", "",
                                handle_poll_report_end, NULL);
    //// poll results
    // generic single value
    lo_server_thread_add_method(st, "/poll/value", "if",
                                handle_poll_value, NULL);
    // generic data blob
    lo_server_thread_add_method(st, "/poll/data", "ib",
                                handle_poll_data, NULL);
    // dedicated path for audio I/O levels
    lo_server_thread_add_method(st, "/poll/vu", "b",
                                handle_poll_io_levels, NULL);
    // tape reports
    lo_server_thread_add_method(st, "/tape/play/state", "s",
                                handle_tape_play_state, NULL);

    lo_server_thread_start(st);
}

void o_deinit(void) {
    fprintf(stderr, "killing audio engine\n");
    lo_send(ext_addr, "/engine/kill", "");
    fprintf(stderr, "stopping OSC server\n");
    lo_server_thread_free(st);    
    lo_address_free(ext_addr);
    lo_address_free(crone_addr);
}

//--- descriptor access
int o_get_num_engines(void) {
    return num_engines;
}

int o_get_num_commands(void) {
    return num_commands;
}

int o_get_num_polls(void) {
    return num_polls;
}

const char **o_get_engine_names(void) {
    return (const char **)engine_names;
}

const struct engine_command *o_get_commands(void) {
    return (const struct engine_command *)commands;
}

const struct engine_poll *o_get_polls(void) {
    return (const struct engine_poll *)polls;
}

//-- mutex access
void o_lock_descriptors() {
    int res = pthread_mutex_lock(&desc_lock);
    if(res) {
        fprintf(stderr, "o_lock_descriptors failed with code %d \b", res);
    }
}

void o_unlock_descriptors() {
    int res = pthread_mutex_unlock(&desc_lock);
    if(res)  {
        fprintf(stderr, "o_unlock_descriptors failed with code %d \b", res);
    }
}

//--- tranmission to audio engine

void o_request_engine_report(void) {
    // fprintf(stderr, "requesting engine report... \n");
    lo_send(ext_addr, "/report/engines", "");
}

void o_load_engine(const char *name) {
    set_need_reports();
    lo_send(ext_addr, "/engine/load/name", "s", name);
}

void o_free_engine() {
    lo_send(ext_addr, "/engine/free", "");
}

void o_send_command(const char *name, lo_message msg) {
    char *path;
    // FIXME: better not to allocate here
    size_t len = sizeof(char) * (strlen(name) + 10);
    path = malloc(len);
    sprintf(path, "/command/%s", name);
    lo_send_message(ext_addr, path, msg);
}

void o_send(const char *name, lo_message msg) {
    lo_send_message(ext_addr, name, msg);
    free(msg);
}

void o_set_poll_state(int idx, bool state) {
    if(state) {
        lo_send(ext_addr, "/poll/start", "i", idx);
    } else {
        lo_send(ext_addr, "/poll/stop", "i", idx);
    }
}

//-------------------------
//--- static function definitions
void o_init_descriptors(void) {
    pthread_mutex_init(&desc_lock, NULL);
    for(int i = 0; i < MAX_NUM_DESC; i++) {
        engine_names[i] = NULL;
        commands[i].name = NULL;
        commands[i].format = NULL;
    }
}

void o_clear_engine_names(void) {
    o_lock_descriptors();
    for(int i = 0; i < num_engines; i++) {
        if(engine_names[i] != NULL) {
            free(engine_names[i]);
            engine_names[i] = NULL;
        } else {
            fprintf(stderr,
                    "o_clear_engine_names: encountered unexpected null entry\n");
        }
    }
    o_unlock_descriptors();
}

void o_clear_commands(void) {
    o_lock_descriptors();
    for(int i = 0; i < num_commands; i++) {
        if( ( commands[i].name != NULL) && ( commands[i].format != NULL) ) {
            free(commands[i].name);
            free(commands[i].format);
            commands[i].name = NULL;
            commands[i].format = NULL;
        } else {
            fprintf(stderr,
                    "o_clear_commands: encountered unexpected null entry\n");
        }
    }
    o_unlock_descriptors();
}

void o_clear_polls(void) {
    o_lock_descriptors();
    for(int i = 0; i < num_polls; i++) {
        if( ( polls[i].name != NULL) ) {
            free(polls[i].name);
            polls[i].name = NULL;
        } else {
            fprintf(stderr,
                    "o_clear_polls: encountered unexpected null entry\n");
        }
    }
    o_unlock_descriptors();
}

// set a given entry in engine name list
void o_set_engine_name(int idx, const char *name) {
    size_t len;
    o_lock_descriptors();
    if(engine_names[idx] != NULL) {
        fprintf(stderr,
                "refusing to allocate engine name %d; already exists",
                idx);
    } else {
        len = strlen(name);
        engine_names[idx] = malloc(len);
        if ( engine_names[idx] == NULL ) {
            fprintf(stderr,
                    "failure to malloc for engine name %d : %s\n",
                    idx,
                    name);
        } else {
            strncpy(engine_names[idx], name, len + 1);
        }
    }
    o_unlock_descriptors();
}

// set a given entry in command list
void o_set_command(int idx, const char *name, const char *format) {
    size_t name_len, format_len;
    o_lock_descriptors();
    if( (commands[idx].name != NULL) || (commands[idx].format != NULL) ) {
        fprintf(stderr,
                "refusing to allocate command name %d; already exists",
                idx);
    } else {
        name_len = strlen(name);
        format_len = strlen(format);
        commands[idx].name = malloc(name_len + 1);
        commands[idx].format = malloc(format_len + 1);
        if ( ( commands[idx].name == NULL) ||
             ( commands[idx].format == NULL) ) {
            fprintf(stderr, "failure to malloc for command %d : %s %s\n",
                    idx,
                    name,
                    format);
        } else {
            strncpy(commands[idx].name, name, name_len + 1);
            strncpy(commands[idx].format, format, format_len + 1);
        }
    }
    o_unlock_descriptors();
}

// set a given entry in polls list
void o_set_poll(int idx, const char *name, poll_type_t type) {
    size_t name_len;
    o_lock_descriptors();
    if( polls[idx].name != NULL ) {
        fprintf(stderr, "refusing to allocate poll name %d; already exists",
                idx);
    } else {
        name_len = strlen(name);
        polls[idx].name = malloc(name_len + 1);
        if ( ( polls[idx].name == NULL) ) {
            fprintf(stderr, "failure to malloc for poll %d : %s\n", idx, name);
        } else {
            strncpy(polls[idx].name, name, name_len + 1);
        }
        polls[idx].type = type;
    }
    o_unlock_descriptors();
}

// set a given descriptor count variable
void o_set_num_desc(int *dst, int num) {
    o_lock_descriptors();
    *dst = num;
    o_unlock_descriptors();
}

// set poll period
void o_set_poll_time(int idx, float dt) {
    lo_send(ext_addr, "/poll/time", "if", idx, dt);
}

// request current value of poll
void o_request_poll_value(int idx) {
    lo_send(ext_addr, "/poll/value", "i", idx);
}

//---- audio context control

//// FIXME: needs 2 levels (OR DOES IT?)
void o_set_audio_input_level(int idx, float level) {
    (void)idx;
    lo_send(crone_addr, "/set/level/adc", "f", level);
}

void o_set_audio_output_level(float level) {
    lo_send(crone_addr, "/set/level/dac", "f", level);
}

void o_set_audio_monitor_level(float level) {    
    lo_send(crone_addr, "/set/level/monitor", "f", level);
}

void o_set_audio_monitor_mono() {
    lo_send(crone_addr, "/set/level/monitor_mix", "iif", 0, 0, 0.5);
    lo_send(crone_addr, "/set/level/monitor_mix", "iif", 0, 1, 0.5);
    lo_send(crone_addr, "/set/level/monitor_mix", "iif", 1, 0, 0.5);
    lo_send(crone_addr, "/set/level/monitor_mix", "iif", 1, 1, 0.5);

}

void o_set_audio_monitor_stereo() {
    lo_send(crone_addr, "/set/level/monitor_mix", "iif", 0, 0, 1.0);
    lo_send(crone_addr, "/set/level/monitor_mix", "iif", 0, 1, 0.0);
    lo_send(crone_addr, "/set/level/monitor_mix", "iif", 1, 0, 0.0);
    lo_send(crone_addr, "/set/level/monitor_mix", "iif", 1, 1, 1.0);
}

void o_set_audio_monitor_on() {
    fprintf(stderr, "o_set_audio_monitor_on() currently unavailable");
}

void o_set_audio_monitor_off() {
    fprintf(stderr, "o_set_audio_monitor_off() currently unavailable");
}

void o_set_audio_pitch_on() {
    lo_send(ext_addr, "/audio/pitch/on", NULL);
}

void o_set_audio_pitch_off() {
    lo_send(ext_addr, "/audio/pitch/off", NULL);
}

void o_restart_audio() {
    lo_send(ext_addr, "/recompile", NULL);
}

//---- tape controls
void o_tape_level(float level) {
    lo_send(ext_addr, "/tape/level", "f", level);
}

void o_tape_new(char *file) {
    lo_send(ext_addr, "/tape/newfile", "s", file);
}

void o_tape_start_rec() {
    lo_send(ext_addr, "/tape/start_rec", NULL);
}

void o_tape_pause_rec() {
    lo_send(ext_addr, "/tape/pause_rec", NULL);
}

void o_tape_stop_rec() {
    lo_send(ext_addr, "/tape/stop_rec", NULL);
}

void o_tape_open(char *file) {
    lo_send(ext_addr, "/tape/openfile", "s", file);
}

void o_tape_play() {
    lo_send(ext_addr, "/tape/play", NULL);
}

void o_tape_pause() {
    lo_send(ext_addr, "/tape/pause", NULL);
}

void o_tape_stop() {
    lo_send(ext_addr, "/tape/stop", NULL);
}

//--- aux effects controls
// enable / disable aux fx processing
void o_set_aux_fx_on() {
    lo_send(crone_addr, "/set/enabled/reverb", "f", 1);
}

void o_set_aux_fx_off() {
    lo_send(crone_addr, "/set/enabled/reverb", "f", 0);
}


//--- insert effects controls
void o_set_insert_fx_on() {
    lo_send(crone_addr, "/set/enabled/compressor", "f", 1);
}

void o_set_insert_fx_off() {
    lo_send(crone_addr, "/set/enabled/compressor", "f", 0);
}

void o_set_insert_fx_mix(float value) {
    lo_send(crone_addr, "/set/level/ins_mix", "f", value);
}


// stereo output -> aux
void o_set_aux_fx_output_level(float value) {
    lo_send(crone_addr, "/set/level/ext/aux", "f", value);
}

// aux return -> dac
void o_set_aux_fx_return_level(float value) {
    lo_send(crone_addr, "/set/level/aux/dac", "f", value);
}

///////////////////////////////
///////////////////////////
///// FIXME EEEEEEEE ???

/// FIXME: doesn't need channel count?
// monitor mix -> aux level (stereo!)
void o_set_aux_fx_input_level(int channel, float value) {
    (void) channel;
    lo_send(crone_addr, "/set/level/monitor/aux", "f", value);
}

// mono input -> aux pan
void o_set_aux_fx_input_pan(int channel, float value) {
    (void)channel;
    (void)value;
    fprintf(stderr, "o_set_aux_fx_input_pan() currently unavailable");}


//// !!!!!!!!
void o_set_aux_fx_param(const char* name, float value) {
    static char buf[128];
    sprintf(buf, "/set/param/reverb/%s", name);
    lo_send(crone_addr, buf, "f", value);
}

/// !!!!!!!!!!!!!!!!!!!
void o_set_insert_fx_param(const char* name, float value) {    static char buf[128];
    sprintf(buf, "/set/param/compressor/%s", name);
    lo_send(crone_addr, buf, "f", value);}


///////////////////
/// TODO OOOOOOOOOO
/*
  /set/level/ext [f]
  /set/level/ext_aux [f]
  /set/level/aux_dac [f]
  /set/level/monitor [f]
  /set/level/monitor_mix [if]
  /set/level/monitor_aux [f]
  /set/level/ins_mix [f]
  /set/enabled/compressor [f]
  /set/enabled/reverb [f]
  /set/param/compressor/ratio [f]
  /set/param/compressor/threshold [f]
  /set/param/compressor/attack [f]
  /set/param/compressor/release [f]
  /set/param/compressor/gain_pre [f]
  /set/param/compressor/gain_post [f]
  /set/param/reverb/pre_del [f]
  /set/param/reverb/lf_fc [f]
  /set/param/reverb/low_rt60 [f]
  /set/param/reverb/mid_rt60 [f]
  /set/param/reverb/hf_damp [f]
  /set/enabled/cut [if]
  /set/level/cut [if]
  /set/pan/cut [if]
  /set/level/adc_cut [f]
  /set/level/ext_cut [f]
  /set/level/cut_aux [f]
  /set/level/in_cut [iif]
  /set/param/cut/rate [if]
  /set/param/cut/loop_start [if]
  /set/param/cut/loop_end [if]
  /set/param/cut/loop_flag [if]
  /set/param/cut/fade_time [if]
  /set/param/cut/rec_level [if]
  /set/param/cut/pre_level [if]
  /set/param/cut/rec_flag [if]
  /set/param/cut/rec_offset [if]
  /set/param/cut/position [if]
  /set/param/cut/filter_fc [if]
  /set/param/cut/filter_fc_mod [if]
  /set/param/cut/filter_rq [if]
  /set/param/cut/filter_lp [if]
  /set/param/cut/filter_hp [if]
  /set/param/cut/filter_bp [if]
  /set/param/cut/filter_br [if]
  /set/param/cut/filter_dry [if]
  /set/param/cut/pre_fade_window [if]
  /set/param/cut/rec_fade_delay [if]
  /set/param/cut/pre_fade_shape [if]
  /set/param/cut/rec_fade_shape [if]
  /set/param/cut/level_slew_time [if]
  /set/param/cut/rate_slew_time [if]
*/


/////////////////////
//////////////////////


///////////////////////////////
/// static function definitions

//---- OSC handlers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

int handle_crone_ready(const char *path,
                       const char *types,
                       lo_arg **argv,
                       int argc,
                       void *data,
                       void *user_data)
{
    norns_hello_ok();
    return 0;
}

int handle_engine_report_start(const char *path,
                               const char *types,
                               lo_arg **argv,
                               int argc,
                               void *data,
                               void *user_data)
{
    assert(argc > 0);
    // arg 1: count of engines
    o_clear_engine_names();
    o_set_num_desc(&num_engines, argv[0]->i);
    return 0;
}

int handle_engine_report_entry(const char *path,
                               const char *types,
                               lo_arg **argv,
                               int argc,
                               void *data,
                               void *user_data) {
    assert(argc > 1);
    // arg 1: engine index
    // arg 2: engine
    // NB: yes, this is the correct way to read a string from a lo_arg!
    o_set_engine_name(argv[0]->i, &argv[1]->s);
    return 0;
}

int handle_engine_report_end(const char *path,
                             const char *types,
                             lo_arg **argv,
                             int argc,
                             void *data,
                             void *user_data) {
    // no arguments; post event
    event_post( event_data_new(EVENT_ENGINE_REPORT) );
    return 0;
}

//---------------------
//--- command report

int handle_command_report_start(const char *path,
                                const char *types,
                                lo_arg **argv,
                                int argc,
                                void *data,
                                void *user_data) {
    assert(argc > 0);
    o_clear_commands();
    o_set_num_desc(&num_commands, argv[0]->i);
    return 0;
}

int handle_command_report_entry(const char *path,
                                const char *types,
                                lo_arg **argv,
                                int argc,
                                void *data,
                                void *user_data) {
    assert(argc > 2);
    o_set_command(argv[0]->i, &argv[1]->s, &argv[2]->s);
    return 0;
}

int handle_command_report_end(const char *path,
                              const char *types,
                              lo_arg **argv,
                              int argc,
                              void *data,
                              void *user_data) {
    needCommandReport = false;
    test_engine_load_done();
    return 0;
}

//---------------------
//--- poll report

int handle_poll_report_start(const char *path,
                             const char *types,
                             lo_arg **argv,
                             int argc,
                             void *data,
                             void *user_data) {

    assert(argc > 0);
    o_clear_polls();
    o_set_num_desc(&num_polls, argv[0]->i);
    return 0;
}

int handle_poll_report_entry(const char *path,
                             const char *types,
                             lo_arg **argv,
                             int argc,
                             void *data,
                             void *user_data) {

    assert(argc > 2);
    fflush(stdout);
    o_set_poll(argv[0]->i, &argv[1]->s, argv[2]->i);
    return 0;
}

int handle_poll_report_end(const char *path, const char *types, lo_arg **argv,
                           int argc, void *data, void *user_data) {

    //event_post( event_data_new(EVENT_POLL_REPORT) );
    needPollReport = false;
    test_engine_load_done();
    return 0;
}

int handle_poll_value(const char *path, const char *types, lo_arg **argv,
                      int argc, void *data, void *user_data) {

    assert(argc > 1);
    union event_data *ev = event_data_new(EVENT_POLL_VALUE);
    ev->poll_value.idx = argv[0]->i;
    ev->poll_value.value = argv[1]->f;
    event_post(ev);
    return 0;
}

int handle_poll_data(const char *path, const char *types, lo_arg **argv,
                     int argc, void *data, void *user_data) {

    assert(argc > 1);
    union event_data *ev = event_data_new(EVENT_POLL_DATA);
    ev->poll_data.idx = argv[0]->i;
    uint8_t *blobdata = (uint8_t *)lo_blob_dataptr( (lo_blob)argv[1] );
    int sz = lo_blob_datasize( (lo_blob)argv[1] );
    ev->poll_data.size = sz;
    ev->poll_data.data = calloc(1, sz);
    memcpy(ev->poll_data.data, blobdata, sz);
    event_post(ev);
    return 0;
}

int handle_poll_io_levels(const char *path, const char *types, lo_arg **argv,
                          int argc, void *data, void *user_data) {

    assert(argc > 0);
    union event_data *ev = event_data_new(EVENT_POLL_IO_LEVELS);
    uint8_t *blobdata = (uint8_t *)lo_blob_dataptr( (lo_blob)argv[0] );
    int sz = lo_blob_datasize( (lo_blob)argv[0] );
    assert( sz == sizeof(quad_levels_t) );
    ev->poll_io_levels.value.uint = *( (uint32_t *)blobdata );
    fflush(stdout);
    event_post(ev);

    return 0;
}

int handle_tape_play_state(const char *path,
                                const char *types,
                                lo_arg **argv,
                                int argc,
                                void *data,
                                void *user_data) {

    //assert(argc > 0);
    //fprintf(stderr, "tape_play_status %s\n", &argv[0]->s);
    return 0;
}


void lo_error_handler(int num, const char *m, const char *path) {
    fprintf(stderr, "liblo error %d in path %s: %s\n", num, path, m);
}

void test_engine_load_done() {
    if( !get_need_reports() ) {
        union event_data *ev = event_data_new(EVENT_ENGINE_LOADED);
        event_post(ev);
    }
}


#pragma GCC diagnostic pop
