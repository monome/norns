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

// wait for a timer to finish
// extern void timer_wait(int idx);

// cancel all scheduled iterations
extern void timer_stop(int idx);

// restart timer immediately

// extern void timer_restart(int idx, double seconds, int count, int stage);
