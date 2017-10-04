#pragma once

#include "device_input.h"
#include "oracle.h"

// initialize the lua VM and run setup scripts
extern void w_init(void);
// stop the VM
extern void w_deinit(void);

// run the user-defined startup routine
extern void w_user_startup(void);

// compile and execute a complete chunk of lua
// this blocks execution and access the VM directly;
// call it only from the main thread
extern void w_run_code(const char *code);

// handle a single line of user input
// this blocks execution and access the VM directly;
// call it only from the main thread
extern void w_handle_line(char *line);

//-------------------------
//---- c -> lua glue

//--- hardware input
extern void w_handle_monome_add(void *dev);
extern void w_handle_monome_remove(int id);
extern void w_handle_grid_key(int id, int x, int y, int state);

extern void w_handle_input_add(void *dev);
extern void w_handle_input_remove(int id);
extern void w_handle_input_event(int id, uint8_t type, dev_code_t code,
                                 int val);

//--- audio engine introspection
extern void w_handle_engine_report(const char **arr, const int num);
extern void w_handle_command_report(const struct engine_command *arr,
                                    const int num);
extern void w_handle_poll_report(const struct engine_poll *arr,
                                    const int num);


//--- timer bang handler
extern void w_handle_timer(const int idx, const int stage);
