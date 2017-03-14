#pragma once

#include "event_types.h"

extern void events_init(void);
extern void event_loop(void);
extern union event_data* event_data_new(event_t evcode);
extern void event_post(union event_data* ev);
