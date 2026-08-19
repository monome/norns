// test stubs for matron/src/lua_shell.cc externs.

#include <cstdlib>
#include <cstring>

#include "system_cmd.h"

#include "lua_shell_stubs.h"

//---------------------------------
//--- test state

const char *g_stub_output = "";
sidecar_result_t g_stub_result = SIDECAR_OK;
bool g_stub_signalled = false;
int g_stub_code = 0;

const char *g_last_cmd = nullptr;
uint32_t g_last_timeout_ms = 0;
int g_call_count = 0;

static char *owned_cmd = nullptr;

void lua_shell_stubs_reset() {
    free(owned_cmd);
    owned_cmd = nullptr;
    g_stub_output = "";
    g_stub_result = SIDECAR_OK;
    g_stub_signalled = false;
    g_stub_code = 0;
    g_last_cmd = nullptr;
    g_last_timeout_ms = 0;
    g_call_count = 0;
}

//---------------------------------
//--- stubbed externs

bool system_cmd_sync(const char *cmd, char **out, size_t *size, sidecar_status_t *status, uint32_t timeout_ms) {
    free(owned_cmd);
    owned_cmd = strdup(cmd);
    g_last_cmd = owned_cmd;
    g_last_timeout_ms = timeout_ms;
    ++g_call_count;

    if (status != nullptr) {
        status->result = g_stub_result;
        status->signalled = g_stub_signalled;
        status->code = g_stub_code;
        status->err = nullptr;
    }

    if (g_stub_result == SIDECAR_TRANSPORT_FAILED) {
        *out = nullptr;
        *size = 0;
        return false;
    }

    *size = strlen(g_stub_output) + 1;
    *out = (char *)malloc(*size);
    memcpy(*out, g_stub_output, *size);
    return true;
}
