#pragma once
#include <stdarg.h>
#include <lo/lo.h>

/*
 * oracle.h
 *
 * this module speaks with the Crone.
 *
 * user should not care about the method (IPC or otherwise.)
 *
 */

#include "lo/lo.h"

// data structure for engine command descriptor/header
struct engine_command {
  char *cmd; // command string
  char *fmt; // format string
};

// initialize
extern void o_init();
// shutdown
extern void o_deinit();

//----------------------
//--- access param and buffer descriptors
// get count of desctipors
extern int o_get_num_engines(void);
extern int o_get_num_commands(void);
//extern int o_get_num_buffers(void);
//extern int o_get_num_params(void);

// get engine names (array of c strings)
extern const char **o_get_engine_names(void);
// get current engine commands (array of structs)
extern const struct engine_command *o_get_commands(void);

/* extern const char** o_get_buffer_names(void); */
/* extern const char** o_get_param_names(void); */

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

// issue a command to the engine
//extern void o_send_command(const char* name, const char* fmt, va_list args);
extern void o_send_command(const char *name, lo_message msg);

//   FIXME: autogenerate from protocol description?
//   use dynamic list of OSC patterns?

/* // request list of buffers */
/* extern void o_request_buffer_report(void); */

/* // load a sound file to named buffer */
/* extern void o_load_buffer_name(const char* name, const char* path); */

/* // save a named buffer to a sound file */
/* extern void o_save_buffer_name(const char* name, const char* path); */

/* // request list of params */
/* extern void o_request_param_report(void); */

/* // set indexed parameter with float value */
/* extern void o_set_param_index(int idx, const float val); */

/* // set named parameter with float value */
/* extern void o_set_param_name(const char* name, const float val); */
