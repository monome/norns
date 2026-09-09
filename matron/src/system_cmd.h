#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sidecar.h"

#define SYSTEM_CMD_CAPTURE_MAX (1024 * 1024)

extern bool system_cmd(const char *cmd, int ref);
extern bool system_cmd_sync(const char *cmd, char **out, size_t *size, sidecar_status_t *status = NULL, uint32_t timeout_ms = SIDECAR_CMD_TIMEOUT_DEFAULT_MS);
extern bool system_cmd_detached(const char *cmd, const char *unit, int ref);
extern bool system_action(const char *action, int ref);

#ifdef NORNS_TEST
extern void *(*system_cmd_test_realloc)(void *, size_t);
#endif
