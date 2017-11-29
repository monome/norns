#pragma once

#include <pthread.h>
#include <stdint.h>
#include <time.h>

extern const int MAX_NUM_METROS;

// intialize the metros system
extern void metros_init(void);

// create a metro at the specified index
// seconds < 0 == use previous period
extern void metro_start(int idx, double seconds, int count, int stage);

// cancel all scheduled iterations
extern void metro_stop(int idx);

// set period of metro
// NB: if the metro is running, its hard to say if new value will take effect
// on current period or next period
extern void metro_set_time(int idx, float sec);
