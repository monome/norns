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
#include "oracle.h"

static lo_address remote_addr;
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

// max count of any single desciptor type
#define MAX_NUM_DESC 1024

char *engine_names[MAX_NUM_DESC];
struct engine_command commands[MAX_NUM_DESC];
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

static void lo_error_handler(int num, const char *m, const char *path);

//-----------------------------------
//---- extern function definitions
int o_ready(void) {
    lo_send(remote_addr, "/ready","");
    return ready;
}

//--- init
void o_init(void) {
    const char *loc_port = args_local_port();
    const char *rem_port = args_remote_port();

    printf("OSC rx port: %s \nOSC tx port: %s\n",
           loc_port, rem_port);
    o_init_descriptors();

    remote_addr = lo_address_new("127.0.0.1", rem_port);
    st = lo_server_thread_new(loc_port, lo_error_handler);

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

    lo_server_thread_start(st);
}

void o_deinit(void) {
    printf("killing audio engine\n"); fflush(stdout);
    lo_send(remote_addr, "/engine/kill", "");
    printf("stopping OSC server\n"); fflush(stdout);
    lo_server_thread_free(st);
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
        printf("o_lock_descriptors failed with code %d \b", res);
        fflush(stdout);
    }
}

void o_unlock_descriptors() {
    int res = pthread_mutex_unlock(&desc_lock);
    if(res)  {
        printf("o_unlock_descriptors failed with code %d \b", res); fflush(
            stdout);
    }
}

//--- tranmission to audio engine

void o_request_engine_report(void) {
    //  printf("requesting engine report... \n"); fflush(stdout);
    lo_send(remote_addr, "/report/engines", "");
}

void o_load_engine(const char *name) {
    /* lo_send(remote_addr, "/engine/load/name", "s", name); */
    /* o_request_command_report(); */
    /* o_request_poll_report(); */
    // how to do it with timestamps:
    lo_timetag now;
    lo_timetag_now(&now);
    lo_send_timestamped(remote_addr, now, "/engine/load/name", "s", name);
    // test with a delay
    // now.frac++ ; // each unit here is 1/32s i think
    lo_send_timestamped(remote_addr, now, "/report/commands", "");
    lo_send_timestamped(remote_addr, now, "/report/polls", "");
}

void o_request_command_report(void) {
    lo_send(remote_addr, "/report/commands", "");
}

void o_request_poll_report(void) {
    // printf("requesting poll report...");  fflush(stdout);
    lo_send(remote_addr, "/report/polls", "");
}

void o_send_command(const char *name, lo_message msg) {
    char *path;
    // FIXME: better not to allocate here
    size_t len = sizeof(char) * (strlen(name) + 10);
    path = malloc(len);
    sprintf(path, "/command/%s", name);
    lo_send_message(remote_addr, path, msg);
    free(msg);
}

void o_send(const char *name, lo_message msg) {
    lo_send_message(remote_addr, name, msg);
    free(msg);
}

void o_set_poll_state(int idx, bool state) {
    if(state) {
        lo_send(remote_addr, "/poll/start", "i", idx);
    } else {
        lo_send(remote_addr, "/poll/stop", "i", idx);
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
            printf("o_clear_engine_names: encountered unexpected null entry \n");
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
            printf("o_clear_commands: encountered unexpected null entry \n");
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
            printf("o_clear_polls: encountered unexpected null entry \n");
        }
    }
    o_unlock_descriptors();
}

// set a given entry in engine name list
void o_set_engine_name(int idx, const char *name) {
    size_t len;
    o_lock_descriptors();
    if(engine_names[idx] != NULL) {
        printf("refusing to allocate engine name %d; already exists", idx);
    } else {
        len = strlen(name);
        engine_names[idx] = malloc(len);
        if ( engine_names[idx] == NULL ) {
            printf("failure to malloc for engine name %d : %s \n", idx, name);
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
        printf("refusing to allocate command name %d; already exists", idx);
    } else {
        name_len = strlen(name);
        format_len = strlen(format);
        commands[idx].name = malloc(name_len + 1);
        commands[idx].format = malloc(format_len + 1);
        if ( ( commands[idx].name == NULL) ||
             ( commands[idx].format == NULL) ) {
            printf("failure to malloc for command %d : %s %s \n",
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
        printf("refusing to allocate poll name %d; already exists", idx);
    } else {
        name_len = strlen(name);
        polls[idx].name = malloc(name_len + 1);
        if ( ( polls[idx].name == NULL) ) {
            printf("failure to malloc for poll %d : %s \n", idx, name);
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
    lo_send(remote_addr, "/poll/time", "if", idx, dt);
}

//---- audio context control

void o_set_audio_input_level(int idx, float level) {
    lo_send(remote_addr, "/audio/input/level", "if", idx, level);
}

void o_set_audio_output_level(float level) {
    lo_send(remote_addr, "/audio/output/level", "f", level);
}

void o_set_audio_monitor_level(float level) {
    lo_send(remote_addr, "/audio/monitor/level", "f", level);
}

void o_set_audio_monitor_mono() {
    lo_send(remote_addr, "/audio/monitor/mono", NULL);
}

void o_set_audio_monitor_stereo() {
    lo_send(remote_addr, "/audio/monitor/stereo", NULL);
}

void o_set_audio_monitor_on() {
    lo_send(remote_addr, "/audio/monitor/on", NULL);
}

void o_set_audio_monitor_off() {
    lo_send(remote_addr, "/audio/monitor/off", NULL);
}

void o_set_audio_pitch_on() {
    lo_send(remote_addr, "/audio/pitch/on", NULL);
}

void o_set_audio_pitch_off() {
    lo_send(remote_addr, "/audio/pitch/off", NULL);
}

///////////////////////////////
/// static function definitions

//---- OSC handlers
int handle_crone_ready(const char *path,
                       const char *types,
                       lo_arg **argv,
                       int argc,
                       void *data,
                       void *user_data)
{
    (void)path;
    (void)types;
    (void)argc;
    (void)argv;
    (void)data;
    (void)user_data;
    ready = 1;
    return 0;
}

int handle_engine_report_start(const char *path,
                               const char *types,
                               lo_arg **argv,
                               int argc,
                               void *data,
                               void *user_data)
{
    (void)path;
    (void)types;
    (void)argc;
    (void)data;
    (void)user_data;
    assert(argc > 0);
    // arg 1: count of buffers
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
    (void)path;
    (void)types;
    (void)argc;
    (void)data;
    (void)user_data;
    assert(argc > 1);
    // arg 1: buffer index
    // arg 2: buffer name
    // NB: yes, this is the correct way to read a string from a lo_arg!
    o_set_engine_name(argv[0]->i, &argv[1]->s);
    return 0;
}

int handle_engine_report_end(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data) {
    (void)path;
    (void)types;
    (void)argc;
    (void)argv;
    (void)data;
    (void)user_data;
    // no arguments; post event
    // FIXME: as yet no outstanding need for report_end message to occur at all.
    // could add counter from report_start to double-check the param count.
    // or (better?) we could simply use binary blobs from Crone,
    // replacing the whole response sequence with a single message
    // (downside: nasty blob-construction code in supercollider?)
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
    (void)path;
    (void)types;
    (void)argc;
    (void)argv;
    (void)data;
    (void)user_data;
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
    (void)path;
    (void)types;
    (void)argc;
    (void)data;
    (void)user_data;
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
    (void)path;
    (void)types;
    (void)argc;
    (void)argv;
    (void)data;
    (void)user_data;
    event_post( event_data_new(EVENT_COMMAND_REPORT) );
    return 0;
}

//---------------------
//--- poll report

int handle_poll_report_start(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data) {
    (void)path;
    (void)types;
    (void)argc;
    (void)argv;
    (void)data;
    (void)user_data;
    assert(argc > 0);
    o_clear_polls();
    o_set_num_desc(&num_polls, argv[0]->i);
    return 0;
}

int handle_poll_report_entry(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data) {
    (void)path;
    (void)types;
    (void)argc;
    (void)data;
    (void)user_data;
    assert(argc > 2);
    fflush(stdout);
    o_set_poll(argv[0]->i, &argv[1]->s, argv[2]->i);
    return 0;
}

int handle_poll_report_end(const char *path, const char *types, lo_arg **argv,
                           int argc, void *data, void *user_data) {
    (void)path;
    (void)types;
    (void)argc;
    (void)argv;
    (void)data;
    (void)user_data;
    event_post( event_data_new(EVENT_POLL_REPORT) );
    return 0;
}

int handle_poll_value(const char *path, const char *types, lo_arg **argv,
                      int argc, void *data, void *user_data) {
    (void)path;
    (void)types;
    (void)argc;
    (void)data;
    (void)user_data;
    union event_data *ev = event_data_new(EVENT_POLL_VALUE);
    ev->poll_value.idx = argv[0]->i;
    ev->poll_value.value = argv[1]->f;
    event_post( ev );
    return 0;
}

int handle_poll_data(const char *path, const char *types, lo_arg **argv,
                     int argc, void *data, void *user_data) {
    (void)path;
    (void)types;
    (void)argc;
    (void)data;
    (void)user_data;
    union event_data *ev = event_data_new(EVENT_POLL_DATA);
    ev->poll_data.idx = argv[0]->i;
    uint8_t *blobdata = (uint8_t *)lo_blob_dataptr( (lo_blob)argv[1] );
    int sz = lo_blob_datasize( (lo_blob)argv[1] );
    ev->poll_data.size = sz;
    ev->poll_data.data = calloc(1, sz);
    memcpy(ev->poll_data.data, blobdata, sz);
    event_post( ev );
    return 0;
}

int handle_poll_io_levels(const char *path, const char *types, lo_arg **argv,
                          int argc, void *data, void *user_data) {
    (void)path;
    (void)types;
    (void)argc;
    (void)data;
    (void)user_data;
    union event_data *ev = event_data_new(EVENT_POLL_IO_LEVELS);
    uint8_t *blobdata = (uint8_t *)lo_blob_dataptr( (lo_blob)argv[0] );
    int sz = lo_blob_datasize( (lo_blob)argv[0] );
    assert( sz == sizeof(quad_levels_t) );
    ev->poll_io_levels.value.uint = *( (uint32_t *)blobdata );
    fflush(stdout);
    event_post( ev );

    return 0;
}

void lo_error_handler(int num, const char *m, const char *path) {
    printf("liblo error %d in path %s: %s\n", num, path, m);
    fflush(stdout);
}
