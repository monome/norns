/*
 * osc.h
 *
 * user OSC device, send/receive arbitrary OSC within lua scripts
 *
 */

#pragma once
#include <stdarg.h>
#include <stdbool.h> 
#include "lo/lo.h"

extern void osc_init();
extern void osc_deinit();

extern int osc_send(const char *,const char *);
