#pragma once

#include <lua.h>

typedef void (*event_custom_weave_op_t)(lua_State *lvm, void *value, void *context);
typedef void (*event_custom_free_op_t)(void *value, void *context);

struct event_custom_ops {
    const char *type_name;
    event_custom_weave_op_t weave;
    event_custom_free_op_t free;
};
