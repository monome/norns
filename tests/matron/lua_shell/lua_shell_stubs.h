// shared declarations for the lua_shell test stubs.

#pragma once

#include <stdint.h>

#include "sidecar.h"

extern const char *g_stub_output;
extern sidecar_result_t g_stub_result;
extern bool g_stub_signalled;
extern int g_stub_code;

extern const char *g_last_cmd;
extern uint32_t g_last_timeout_ms;
extern int g_call_count;

void lua_shell_stubs_reset();
