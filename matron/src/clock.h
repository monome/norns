#pragma once

#include <lua.h>

void clock_init();
bool clock_schedule_resume_sleep(int thread_id, double seconds);
bool clock_schedule_resume_sync(int thread_id, double beats);
void clock_update_counter(int beats, double beat_duration);
void clock_cancel_all();

double clock_gettime_beats();
double clock_gettime_secondsf();

void clock_cancel(int);
void clock_cancel_coro(int);
