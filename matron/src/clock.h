#pragma once

#include <lua.h>

void clock_init();
void clock_schedule_resume(int thread_id, float seconds);
int clock_counter_increment();
int clock_counter_get();
void clock_sync(float quant);
