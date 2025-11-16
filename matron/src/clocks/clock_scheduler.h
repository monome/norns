#pragma once

#include <stdbool.h>

#define NUM_CLOCK_SCHEDULER_EVENTS 100

void clock_scheduler_init();
void clock_scheduler_start();
void clock_scheduler_stop();
void clock_scheduler_join();
bool clock_scheduler_schedule_sync(int thread_id, double sync_beat, double sync_beat_offset);
bool clock_scheduler_schedule_sleep(int thread_id, double seconds);
void clock_scheduler_clear(int thread_id);
void clock_scheduler_clear_all();
void clock_scheduler_reschedule_sync_events();
void clock_scheduler_reset_sync_events();

#ifdef NORNS_TEST
double clock_scheduler_test_next_clock_beat(double clock_beat, double sync_beat, double sync_beat_offset);
#endif
