#pragma once

#include "device_hid.h"
#include "oracle.h"
#include "event_types.h"

// initialize the lua VM and run setup scripts
extern void w_init(void);
// stop the VM
extern void w_deinit(void);

// run the startup/post-startup routines
extern void w_startup(void);
extern void w_post_startup(void);

// compile and execute a complete chunk of lua
// this blocks execution and access the VM directly;
// call it only from the main thread
extern void w_run_code(const char *code);

// handle a single line of user input
// this blocks execution and access the VM directly;
// call it only from the main thread
extern void w_handle_exec_code_line(char *line);

// reset the lua state machine
extern void w_reset_lvm();

//-------------------------
//---- c -> lua glue

//--- hardware input
extern void w_handle_monome_add(void *dev);
extern void w_handle_monome_remove(int id);
extern void w_handle_grid_key(int id, int x, int y, int state);
extern void w_handle_grid_tilt(int id, int sensor, int x, int y, int z);
extern void w_handle_arc_encoder_delta(int id, int number, int delta);
extern void w_handle_arc_encoder_key(int id, int number, int state);

extern void w_handle_hid_add(void *dev);
extern void w_handle_hid_remove(int id);
extern void w_handle_hid_event(int id, uint8_t type, dev_code_t code, int val);

extern void w_handle_midi_add(void *dev);
extern void w_handle_midi_remove(int id);
extern void w_handle_midi_event(int id, uint8_t *data, size_t nbytes);

extern void w_handle_crow_add(void *dev);
extern void w_handle_crow_remove(int id);
extern void w_handle_crow_event(void *dev, int id);

extern void w_handle_serial_add(void *dev);
extern void w_handle_serial_remove(uint32_t id, char *handler_id);
extern void w_handle_serial_event(void *dev, uint32_t id);

extern void w_handle_serial_config(char *path, char *name, char *vendor, char *model, char *serial, char *interface);

extern void w_handle_osc_event(char *from_host, char *from_port, char *path, lo_message msg);

//--- audio engine introspection
extern void w_handle_engine_report(const char **arr, const int num);
extern void w_handle_engine_load_done();

//--- gpio handler
extern void w_handle_key(const int n, const int val);
extern void w_handle_enc(const int n, const int delta);

//--- system/battery
extern void w_handle_battery(const int percent, const int current);
extern void w_handle_power(const int present);

//--- system/stat
extern void w_handle_stat(const uint32_t disk, const uint16_t temp, const uint16_t cpu,
    const uint16_t cpu1, const uint16_t cpu2, const uint16_t cpu3, const uint16_t cpu4);

//--- metro bang handler
extern void w_handle_metro(const int idx, const int stage);

//--- clock
extern void w_handle_clock_resume(const int thread_id, double value);
extern void w_handle_clock_start();
extern void w_handle_clock_stop();

//--- crone poll handlers
extern void w_handle_poll_value(int idx, float val);
extern void w_handle_poll_data(int idx, int size, uint8_t *data);
extern void w_handle_poll_wave(int idx, uint8_t *data);
extern void w_handle_poll_io_levels(uint8_t *levels);
extern void w_handle_poll_softcut_phase(int idx, float val);
extern void w_handle_softcut_render(int idx, float sec_per_sample, float start, size_t size, float* data);
extern void w_handle_softcut_position(int idx, float pos);

extern void w_handle_engine_loaded();

// callbacks for ACK and timeout of sclang startup
extern void w_handle_startup_ready_ok();
extern void w_handle_startup_ready_timeout();

// util callbacks
extern void w_handle_system_cmd();

// display driver callbacks
extern void w_handle_screen_refresh();

// custom events
extern void w_handle_custom_weave(struct event_custom *ev);
