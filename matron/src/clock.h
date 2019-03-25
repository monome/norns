#pragma once

#include <lua.h>

void clock_init();
void clock_schedule_resume(int thread_id, float seconds);
void clock_schedule_resume_sync(int thread_id, float beats);
void clock_counter_increment(float beat_duration);
int clock_counter_get();
void clock_sync(float quant);
void clock_cancel_all();
float clock_gettime_beats();
