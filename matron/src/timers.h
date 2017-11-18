#pragma once

#include <pthread.h>
#include <stdint.h>
#include <time.h>

extern const int MAX_NUM_TIMERS;

// intialize the timers system
extern void timers_init(void);

// create a timer at the specified index
// seconds < 0 == use previous period
extern void timer_start(int idx, double seconds, int count, int stage);

// cancel all scheduled iterations
extern void timer_stop(int idx);

// set period of timer
// NB: if the timer is running, its hard to say if new value will take effect
// on current period or next period
extern void timer_set_time(int idx, float sec);
