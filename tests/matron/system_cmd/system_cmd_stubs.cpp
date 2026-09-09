// test stubs for matron/src/system_cmd.cc externs.

#include <cstdlib>
#include <cstring>

#include "events.h"
#include "sidecar.h"
#include "system_cmd.h"

#include "system_cmd_stubs.h"

//---------------------------------
//--- test state

const char *g_last_cmd = nullptr;
void *g_last_ctx = nullptr;
sidecar_chunk_cb_t g_last_on_chunk = nullptr;
sidecar_done_cb_t g_last_on_done = nullptr;

const char *g_last_unit = nullptr;

static char *owned_cmd = nullptr;
static char *owned_unit = nullptr;

static void capture_cmd(const char *cmd) {
    free(owned_cmd);
    owned_cmd = strdup(cmd);
    g_last_cmd = owned_cmd;
}

static void capture_unit(const char *unit) {
    free(owned_unit);
    owned_unit = strdup(unit);
    g_last_unit = owned_unit;
}

const char *g_sync_chunks[SYNC_SCRIPT_MAX_CHUNKS] = {};
int g_sync_chunk_count = 0;
sidecar_result_t g_sync_result = SIDECAR_OK;
const char *g_sync_errmsg = nullptr;
bool g_sync_signalled = false;
int g_sync_code = 0;

uint32_t g_last_timeout_ms = 0;

bool g_cmd_async_result = true;

union event_data *g_last_event = nullptr;
int g_event_post_count = 0;

void system_cmd_stubs_reset() {
    free(owned_cmd);
    owned_cmd = nullptr;
    free(owned_unit);
    owned_unit = nullptr;
    g_last_cmd = nullptr;
    g_last_ctx = nullptr;
    g_last_on_chunk = nullptr;
    g_last_on_done = nullptr;
    g_last_unit = nullptr;
    for (int i = 0; i < SYNC_SCRIPT_MAX_CHUNKS; ++i) {
        g_sync_chunks[i] = nullptr;
    }
    g_sync_chunk_count = 0;
    g_sync_result = SIDECAR_OK;
    g_sync_errmsg = nullptr;
    g_sync_signalled = false;
    g_sync_code = 0;
    g_last_timeout_ms = 0;
    g_cmd_async_result = true;
    g_last_event = nullptr;
    g_event_post_count = 0;
    system_cmd_test_realloc = realloc;
}

//---------------------------------
//--- stubbed externs

bool sidecar_client_cmd_async(const char *cmd, uint32_t timeout_ms, void *ctx, sidecar_chunk_cb_t on_chunk, sidecar_done_cb_t on_done) {
    capture_cmd(cmd);
    g_last_timeout_ms = timeout_ms;
    g_last_ctx = ctx;
    g_last_on_chunk = on_chunk;
    g_last_on_done = on_done;
    return g_cmd_async_result;
}

bool sidecar_client_detach_async(const char *cmd, const char *unit, void *ctx, sidecar_done_cb_t on_done) {
    capture_cmd(cmd);
    capture_unit(unit);
    g_last_ctx = ctx;
    g_last_on_done = on_done;
    return true;
}

void sidecar_client_cmd(const char *cmd, uint32_t timeout_ms, void *ctx, sidecar_chunk_cb_t on_chunk, sidecar_done_cb_t on_done) {
    capture_cmd(cmd);
    g_last_timeout_ms = timeout_ms;
    g_last_ctx = ctx;
    g_last_on_chunk = on_chunk;
    g_last_on_done = on_done;
    for (int i = 0; i < g_sync_chunk_count; ++i) {
        on_chunk(ctx, g_sync_chunks[i], std::strlen(g_sync_chunks[i]));
    }
    const sidecar_status_t status = {g_sync_result, g_sync_signalled, g_sync_code, g_sync_errmsg};
    on_done(cmd, ctx, &status);
}

union event_data *event_data_new(event_t evcode) {
    auto *ev = (union event_data *)calloc(1, sizeof(union event_data));
    ev->type = evcode;
    return ev;
}

void event_post(union event_data *ev) {
    g_last_event = ev;
    ++g_event_post_count;
}
