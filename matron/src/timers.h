#pragma once

#include <pthread.h>
#include <stdint.h>
#include <time.h>

extern const int MAX_NUM_TIMERS;

typedef unsigned long long int counter_t;
typedef unsigned long long int period_t;
typedef void(*timer_cb)(int id, counter_t count);

extern void timer_add(int idx, timer_cb cb, double seconds, int count);
extern void timer_wait(int idx);

// TODO
// extern void timer_stop(int idx);
// extern void timer_restart(int idx);
// extern void timer_set_period(int idx, double seconds);
// extern void timer_set_callback(int idx, timer_cb cb);
