#pragma once

#include <stdbool.h>

#define NUM_CLOCK_SCHEDULER_EVENTS 100

void clock_scheduler_init();
void clock_scheduler_start();
bool clock_scheduler_schedule_sync(int thread_id, double beats);
bool clock_scheduler_schedule_sleep(int thread_id, double seconds);
void clock_scheduler_reschedule_sync_events();
