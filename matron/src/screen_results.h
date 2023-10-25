#pragma once

#include "event_types.h"

extern void screen_results_init();
extern void screen_results_wait();
extern void screen_results_post(union event_data *ev);
extern union event_data *screen_results_get();