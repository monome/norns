#pragma once

#include <lua.h>

extern void clock_init();
extern void clock_schedule(int thread_id, float seconds);
