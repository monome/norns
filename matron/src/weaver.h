#pragma once

#include "device_hid.h"
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

extern void w_handle_hid_add(void *dev);
extern void w_handle_hid_remove(int id);
extern void w_handle_hid_event(int id, uint8_t type, dev_code_t code,
                                 int val);

//--- audio engine introspection
extern void w_handle_engine_report(const char **arr, const int num);
extern void w_handle_command_report(const struct engine_command *arr,
                                    const int num);
extern void w_handle_poll_report(const struct engine_poll *arr,
                                 const int num);

//--- gpio handler
extern void w_handle_key(const int n, const int val);
extern void w_handle_enc(const int n, const int delta);

//--- system/battery
extern void w_handle_battery(const int percent);
extern void w_handle_power(const int present);

//--- metro bang handler
extern void w_handle_metro(const int idx, const int stage);

//--- crone poll handlers
extern void w_handle_poll_value(int idx, float val);
extern void w_handle_poll_data(int idx, int size, uint8_t *data);
