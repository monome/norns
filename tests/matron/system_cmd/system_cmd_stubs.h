// shared declarations for the system_cmd test stubs.

#pragma once

#include "events.h"
#include "sidecar.h"

extern const char *g_last_cmd;
extern void *g_last_ctx;
extern sidecar_chunk_cb_t g_last_on_chunk;
extern sidecar_done_cb_t g_last_on_done;

extern const char *g_last_unit;

enum { SYNC_SCRIPT_MAX_CHUNKS = 8 };
extern const char *g_sync_chunks[SYNC_SCRIPT_MAX_CHUNKS];
extern int g_sync_chunk_count;
extern sidecar_result_t g_sync_result;
extern const char *g_sync_errmsg;
extern bool g_sync_signalled;
extern int g_sync_code;

extern uint32_t g_last_timeout_ms;

extern bool g_cmd_async_result;

extern union event_data *g_last_event;
extern int g_event_post_count;

void system_cmd_stubs_reset();
