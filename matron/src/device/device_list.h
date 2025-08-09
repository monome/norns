#pragma once

#include "device.h"
#include <lualib.h>

extern void dev_list_init(void);
extern void dev_list_add(device_t type, const char *node, const char *name, lua_State *l);
extern void dev_list_remove(device_t type, const char *node);
