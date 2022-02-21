#pragma once
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