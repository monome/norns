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

// check for audio engine boot completion
extern int o_ready();
// initialize
extern void o_init();
// shutdown
extern void o_deinit();

//----------------------
//--- access param and buffer descriptors
// get count of desctipors
extern int o_get_num_engines(void);
extern int o_get_num_commands(void);
extern int o_get_num_polls(void);

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

// request list of engine names
extern void o_request_engine_report(void);

// request list of commands
extern void o_request_command_report(void);

// request list of polls
extern void o_request_poll_report(void);

// issue a command to the engine, adds /command/ pattern
// NB: this requires a pre-allocated lo_message, which will be freed!
extern void o_send_command(const char *name, lo_message msg);
// send osc message 
// NB: this requires a pre-allocated lo_message, which will be freed!
extern void o_send(const char *name, lo_message msg);


// start or stop a poll
//extern void o_set_poll_state(const char *name, bool state);
extern void o_set_poll_state(int idx, bool state);

// set poll period
extern void o_set_poll_time(int idx, float dt);
