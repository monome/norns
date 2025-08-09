#pragma once
#include <stdbool.h>
#include <stdint.h>

// exposed for the clock module?
extern double jack_sample_rate;

// initialize jack client
// returns 0 on success
extern int jack_client_init();

extern void jack_client_deinit();

// return the running estimate of audio CPU load reported by jack
// returns ratio in [0,1]
extern float jack_client_get_cpu_load();

// update and return estimate of undderun count since last query
extern uint32_t jack_client_get_xrun_count();

// get JACks current system time estimate in seconds (computed from sample frames)
extern double jack_client_get_current_time();

extern const char **jack_client_get_input_ports();
extern const char **jack_client_get_output_ports();
extern const char **jack_client_get_port_connections(const char *port_name);
extern void jack_client_free_port_list(const char **list);
extern bool jack_client_connect(const char *source_name, const char *destination_name);
extern bool jack_client_disconnect(const char *source_name, const char *destination_name);
