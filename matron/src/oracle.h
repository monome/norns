#pragma once

/* 
oracle.h

this module speaks with the Crone. 

user should not care about the method (IPC or otherwise.)

 */

#include "lo/lo.h"

// initialize
extern void o_init();

//-----------------------
//--- IPC functions

//   FIXME: autogenerate from protcol description?
//   use dynamic list of OSC patterns?

// load a processing module
extern void o_load_module(const char* name);

// load a sound file to a named buffer
extern void o_load_buffer(const char* name, const char* path);

// set indexed parameter with float value
extern void o_set_param_index(int idx, const float val);

// set named parameter with float value
extern void o_set_param_name(const char* name, const float val);

