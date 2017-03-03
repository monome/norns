#pragma once

#include <pthread.h>
#include <stdint.h>
#include <time.h>

extern const int MAX_NUM_TIMERS;

// intialize the timers system
extern void timers_init(void);

// create a timer at the specified index
extern void timer_start(int idx, double seconds, int count);
// wait for a timer to finish
extern void timer_wait(int idx);
// cancel all scheduled iterations
extern void timer_stop(int idx);

// TODO
// restart counter immediately (at an arbitrary position?)
// extern void timer_reset(int idx, int count);
// extern void timer_set_period(int idx, double seconds);
// extern void timer_set_callback(int idx, timer_cb cb);
