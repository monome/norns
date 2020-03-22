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

static int handle_poll_softcut_phase(const char *path, const char *types,
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

    fprintf(stderr, "OSC rx port: %s \nOSC crone port: %s\nOSC ext port: %s\n",
            local_port, crone_port, ext_port);
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
    // softcut polls
    lo_server_thread_add_method(st, "/poll/softcut/phase", "if",
                                handle_poll_softcut_phase, NULL);
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
    free(path);
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
        len = strlen(name) + 1; // include null terminator
        engine_names[idx] = malloc(len);
        if ( engine_names[idx] == NULL ) {
            fprintf(stderr,
                    "failure to malloc for engine name %d : %s\n",
                    idx,
                    name);
        } else {
            strncpy(engine_names[idx], name, len);
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
    lo_send(ext_addr, "/poll/request/value", "i", idx);
}

//---- audio context control

void o_poll_start_vu() {
    lo_send(crone_addr, "/poll/start/vu", NULL);
}

void o_poll_stop_vu() {
    lo_send(crone_addr, "/poll/stop/vu", NULL);
}

void o_poll_start_cut_phase() {
    lo_send(crone_addr, "/poll/start/cut/phase", NULL);
}

void o_poll_stop_cut_phase() {
    lo_send(crone_addr, "/poll/stop/cut/phase", NULL);
}



void o_set_level_adc(float level) {
    lo_send(crone_addr, "/set/level/adc", "f", level);
}

void o_set_level_dac(float level) {
    lo_send(crone_addr, "/set/level/dac", "f", level);
}

void o_set_level_ext(float level) {
    lo_send(crone_addr, "/set/level/ext", "f", level);
}

void o_set_level_monitor(float level) {    
    lo_send(crone_addr, "/set/level/monitor", "f", level);
}

void o_set_monitor_mix_mono() {
    lo_send(crone_addr, "/set/level/monitor_mix", "if", 0, 0.5);
    lo_send(crone_addr, "/set/level/monitor_mix", "if", 1, 0.5);
    lo_send(crone_addr, "/set/level/monitor_mix", "if", 2, 0.5);
    lo_send(crone_addr, "/set/level/monitor_mix", "if", 3, 0.5);
}

void o_set_monitor_mix_stereo() {
    lo_send(crone_addr, "/set/level/monitor_mix", "if", 0, 1.0);
    lo_send(crone_addr, "/set/level/monitor_mix", "if", 1, 0.0);
    lo_send(crone_addr, "/set/level/monitor_mix", "if", 2, 0.0);
    lo_send(crone_addr, "/set/level/monitor_mix", "if", 3, 1.0);
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
void o_set_level_tape(float level) {
    lo_send(crone_addr, "/set/level/tape", "f", level);
}

void o_set_level_tape_rev(float level) {
    lo_send(crone_addr, "/set/level/tape_rev", "f", level);
}

void o_tape_rec_open(char *file) {
    lo_send(crone_addr, "/tape/record/open", "s", file);
}

void o_tape_rec_start() {
    lo_send(crone_addr, "/tape/record/start", NULL);
}

void o_tape_rec_stop() {
    lo_send(crone_addr, "/tape/record/stop", NULL);
}

void o_tape_play_open(char *file) {
    lo_send(crone_addr, "/tape/play/open", "s", file);
}

void o_tape_play_start() {
    lo_send(crone_addr, "/tape/play/start", NULL);
}

void o_tape_play_stop() {
    lo_send(crone_addr, "/tape/play/stop", NULL);
}


//--- cut
void o_cut_enable(int i, float value) {
    lo_send(crone_addr, "/set/enabled/cut", "if", i, value);
}

void o_set_level_adc_cut(float value) {
    lo_send(crone_addr, "/set/level/adc_cut", "f", value);
}

void o_set_level_ext_cut(float value) {
    lo_send(crone_addr, "/set/level/ext_cut", "f", value);
}

void o_set_level_tape_cut(float value) {
    lo_send(crone_addr, "/set/level/tape_cut", "f", value);
}

void o_set_level_cut_rev(float value) {
    lo_send(crone_addr, "/set/level/cut_rev", "f", value);
}

void o_set_level_cut_master(float value) {
    lo_send(crone_addr, "/set/level/cut_master", "f", value);
}

void o_set_level_cut(int index, float value) {
	lo_send(crone_addr, "/set/level/cut", "if", index, value);
}

void o_set_level_cut_cut(int src, int dest, float value) {
	lo_send(crone_addr, "/set/level/cut_cut", "iif", src, dest, value);
}

void o_set_pan_cut(int index, float value) {
	lo_send(crone_addr, "/set/pan/cut", "if", index, value);
}

void o_set_cut_param(const char* name, int voice, float value) {
    static char buf[128];
    sprintf(buf, "/set/param/cut/%s", name);
    lo_send(crone_addr, buf, "if", voice, value);
}

void o_set_cut_param_ii(const char* name, int voice, int value) {
    static char buf[128];
    sprintf(buf, "/set/param/cut/%s", name);
    lo_send(crone_addr, buf, "ii", voice, value);
}

void o_set_cut_param_iif(const char* name, int a, int b, float v) {
    static char buf[128];
    sprintf(buf, "/set/param/cut/%s", name);    
    lo_send(crone_addr, buf, "iif", a, b, v);
}

void o_set_level_input_cut(int src, int dst, float level) {
    lo_send(crone_addr, "/set/level/in_cut", "iif", src, dst, level);
}

void o_cut_buffer_clear() {
    lo_send(crone_addr, "/softcut/buffer/clear", "");
}

void o_cut_buffer_clear_channel(int ch) {
    lo_send(crone_addr, "/softcut/buffer/clear_channel", "i", ch);
}

void o_cut_buffer_clear_region(float start, float end) {
    lo_send(crone_addr, "/softcut/buffer/clear_region", "ff", start, end);
}

void o_cut_buffer_clear_region_channel(int ch, float start, float end) {
    lo_send(crone_addr, "/softcut/buffer/clear_region_channel", "iff", ch, start, end);
}

void o_cut_buffer_read_mono(char *file, float start_src, float start_dst,
    float dur, int ch_src, int ch_dst) {
    lo_send(crone_addr, "/softcut/buffer/read_mono", "sfffii", file, start_src,
	    start_dst, dur, ch_src, ch_dst);
} 

void o_cut_buffer_read_stereo(char *file, float start_src, float start_dst, float dur) {
    lo_send(crone_addr, "/softcut/buffer/read_stereo", "sfff", file, start_src, start_dst, dur);
} 

void o_cut_buffer_write_mono(char *file, float start, float dur, int ch) {
    lo_send(crone_addr, "/softcut/buffer/write_mono", "sffi", file, start, dur, ch);
}

void o_cut_buffer_write_stereo(char *file, float start, float dur) {
    lo_send(crone_addr, "/softcut/buffer/write_stereo", "sff", file, start, dur);
}

void o_cut_reset() {
    lo_send(crone_addr, "/softcut/reset", "");
}

//--- rev effects controls
// enable / disable rev fx processing
void o_set_rev_on() {
    lo_send(crone_addr, "/set/enabled/reverb", "f", 1.0);
}

void o_set_rev_off() {
    lo_send(crone_addr, "/set/enabled/reverb", "f", 0.0);
}


//--- comp effects controls
void o_set_comp_on() {
    lo_send(crone_addr, "/set/enabled/compressor", "f", 1.0);
}

void o_set_comp_off() {
    lo_send(crone_addr, "/set/enabled/compressor", "f", 0.0);
}

void o_set_comp_mix(float value) {
    lo_send(crone_addr, "/set/level/compressor_mix", "f", value);
}


// stereo output -> rev
void o_set_level_ext_rev(float value) {
    lo_send(crone_addr, "/set/level/ext_rev", "f", value);
}

// rev return -> dac
void o_set_level_rev_dac(float value) {
    lo_send(crone_addr, "/set/level/rev_dac", "f", value);
}

// monitor mix -> rev level
void o_set_level_monitor_rev(float value) {
    lo_send(crone_addr, "/set/level/monitor_rev", "f", value);
}

void o_set_rev_param(const char* name, float value) {
    static char buf[128];
    sprintf(buf, "/set/param/reverb/%s", name);
    lo_send(crone_addr, buf, "f", value);
}

void o_set_comp_param(const char* name, float value) {
    static char buf[128];
    sprintf(buf, "/set/param/compressor/%s", name);
    lo_send(crone_addr, buf, "f", value);
}



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

int handle_poll_softcut_phase(const char *path, const char *types, lo_arg **argv,
                          int argc, void *data, void *user_data) {

    assert(argc > 1);
    union event_data *ev = event_data_new(EVENT_POLL_SOFTCUT_PHASE);
    ev->softcut_phase.idx = argv[0]->i;
    ev->softcut_phase.value = argv[1]->f;
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
