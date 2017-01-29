#pragma once

/* 
oracle.h

this module speaks with the Crone. 

user should not care about the method (IPC or otherwise.)

 */

#include "lo/lo.h"

// initialize
extern void o_init();
// shutdown
extern void o_deinit();

//----------------------
//--- access param and buffer descriptors
// get count of desctipors
extern int o_get_num_buffers(void);
extern int o_get_num_engines(void);
extern int o_get_num_params(void);

// get descriptor data (just arrays of c strings for now)
extern const char** o_get_buffer_names(void);
extern const char** o_get_engine_names(void);
extern const char** o_get_param_names(void);


// lock/unlock the descriptor critical section
// FIXME: not ideal, but any use of the accessors above
// must be bracketed by these
extern void o_lock_descriptors();
extern void o_unlock_descriptors();

//-----------------------
//--- IPC functions

// load a named audio processing engine
extern void o_load_engine(const char* name);
// request list of engine names
extern void o_request_engine_report(void);

//   FIXME: autogenerate from protcol description?
//   use dynamic list of OSC patterns?

// load a sound file to a named buffer
extern void o_load_buffer_name(const char* name, const char* path);

// set indexed parameter with float value
extern void o_set_param_index(int idx, const float val);

// set named parameter with float value
extern void o_set_param_name(const char* name, const float val);

