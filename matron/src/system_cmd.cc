#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "events.h"
#include "sidecar.h"

#include "system_cmd.h"

//----------------------------
//--- types and variables

#ifdef NORNS_TEST
void *(*system_cmd_test_realloc)(void *, size_t) = realloc;
#define CAPTURE_REALLOC system_cmd_test_realloc
#else
#define CAPTURE_REALLOC realloc
#endif

// accumulated command output, capped at SYSTEM_CMD_CAPTURE_MAX
typedef struct {
    char *buf;
    size_t len;
    bool failed;
    bool truncated;
    int cb_ref;              // unused on the sync path
    sidecar_status_t status; // sync path only
} capture_t;

static void capture_append(void *ctx, const char *chunk, size_t size) {
    capture_t *cap = (capture_t *)ctx;
    if (cap->truncated) {
        return;
    }
    if (size > SYSTEM_CMD_CAPTURE_MAX - cap->len) {
        size = SYSTEM_CMD_CAPTURE_MAX - cap->len;
        cap->truncated = true;
        fprintf(stderr, "system_cmd: output exceeded cap, truncating\n");
    }
    if (size == 0) {
        return;
    }
    char *grown = (char *)CAPTURE_REALLOC(cap->buf, cap->len + size);
    if (grown == NULL) {
        cap->truncated = true;
        fprintf(stderr, "system_cmd: out of memory, truncating\n");
        return;
    }
    memcpy(grown + cap->len, chunk, size);
    cap->buf = grown;
    cap->len += size;
}

static void capture_discard_on_transport_failure(capture_t *cap, sidecar_result_t result) {
    if (result != SIDECAR_TRANSPORT_FAILED) {
        return;
    }
    cap->failed = true;
    free(cap->buf);
    cap->buf = NULL;
    cap->len = 0;
}

static void capture_null_terminate(capture_t *cap) {
    char *out = (char *)CAPTURE_REALLOC(cap->buf, cap->len + 1);
    if (out == NULL) {
        fprintf(stderr, "system_cmd: out of memory, dropping output\n");
        free(cap->buf);
        cap->truncated = true;
        cap->len = 0;
        out = (char *)calloc(1, 1);
        if (out == NULL) {
            cap->buf = NULL;
            return;
        }
    }
    out[cap->len] = '\0';
    cap->buf = out;
    cap->len += 1;
}

// FIXME the settings password change passes a plaintext password through here,
// and stderr reaches the journal and maiden. fixing it needs a way to pass a
// secret on stdin rather than in the command string
static void log_cmd_failure(const char *label, const char *cmd, const char *err) {
    fprintf(stderr, "%s: command (%s) failed: %s\n", label, cmd != NULL ? cmd : "", err != NULL ? err : "unknown error");
}

static void post_cmd_done(const char *cmd, void *ctx, const sidecar_status_t *status) {
    capture_t *cap = (capture_t *)ctx;
    capture_discard_on_transport_failure(cap, status->result);

    if (status->result == SIDECAR_TRANSPORT_FAILED) {
        log_cmd_failure("system_cmd", cmd, status->err);
    }

    // FIXME failure and silent success both arrive as "". telling them apart
    // needs an extra callback argument, which changes the signature every
    // script's norns.system_cmd callback is written against
    capture_null_terminate(cap);
    union event_data *ev = event_data_new(EVENT_SYSTEM_CMD);
    ev->system_cmd.capture = cap->buf; // freed when the event is handled
    ev->system_cmd.cb_ref = cap->cb_ref;
    event_post(ev);
    free(cap);
}

static void finish_sync_capture(const char *cmd, void *ctx, const sidecar_status_t *status) {
    (void)cmd;
    capture_t *cap = (capture_t *)ctx;
    capture_discard_on_transport_failure(cap, status->result);
    cap->status = *status;
    cap->status.err = NULL;
}

static void post_launch_done(const char *cmd, void *ctx, const sidecar_status_t *status) {
    const int cb_ref = (int)(intptr_t)ctx;
    const bool ok = (status->result == SIDECAR_OK);
    const char *err = status->err;

    if (status->result == SIDECAR_TRANSPORT_FAILED) {
        err = "no answer from the sidecar, the action may still have started";
    }

    if (!ok) {
        log_cmd_failure("system_cmd_detached", cmd, err);
    }

    union event_data *ev = event_data_new(EVENT_SYSTEM_CMD_DONE);
    if (err != NULL) {
        ev->system_cmd_done.err = strdup(err);
    }
    ev->system_cmd_done.cb_ref = cb_ref;
    ev->system_cmd_done.ok = ok;
    event_post(ev);
}

typedef struct {
    const char *name;
    const char *cmd;
    const char *unit;
} system_action_t;

static const system_action_t system_actions[] = {
    {"shutdown", "sleep 0.5; sudo shutdown now", "norns-shutdown"},
    {"reset", "sudo systemctl restart norns-sclang.service norns-main.service", "norns-reset"},
    {"update", NULL, "norns-update"},
};

static void build_update_cmd(char *buf, size_t len) {
    const char *home = getenv("HOME");
    if (home == NULL || home[0] == '\0') {
        home = "/home/we";
    }
    snprintf(buf, len, "/bin/bash %s/norns/update/update.sh", home);
}

//-------------------------------
//-- extern function definitions

bool system_cmd(const char *cmd, int cb_ref) {
    capture_t *cap = (capture_t *)calloc(1, sizeof(capture_t));
    if (cap == NULL) {
        return false;
    }
    cap->cb_ref = cb_ref;
    if (!sidecar_client_cmd_async(cmd, SIDECAR_CMD_TIMEOUT_DEFAULT_MS, cap, capture_append, post_cmd_done)) {
        free(cap);
        return false;
    }
    return true;
}

bool system_cmd_sync(const char *cmd, char **out, size_t *size, sidecar_status_t *status, uint32_t timeout_ms) {
    capture_t cap = {};
    sidecar_client_cmd(cmd, timeout_ms, &cap, capture_append, finish_sync_capture);
    if (status != NULL) {
        *status = cap.status;
    }
    if (cap.failed) {
        *out = NULL;
        *size = 0;
        return false;
    }
    capture_null_terminate(&cap);
    *out = cap.buf;
    *size = cap.len;
    return true;
}

bool system_cmd_detached(const char *cmd, const char *unit, int cb_ref) {
    return sidecar_client_detach_async(cmd, unit, (void *)(intptr_t)cb_ref, post_launch_done);
}

bool system_action(const char *action, int cb_ref) {
    for (size_t i = 0; i < sizeof(system_actions) / sizeof(system_actions[0]); ++i) {
        const system_action_t *a = &system_actions[i];
        if (strcmp(a->name, action) != 0) {
            continue;
        }
        if (a->cmd != NULL) {
            return system_cmd_detached(a->cmd, a->unit, cb_ref);
        }
        char cmd[512];
        build_update_cmd(cmd, sizeof(cmd));
        return system_cmd_detached(cmd, a->unit, cb_ref);
    }
    fprintf(stderr, "system_action: unknown action (%s)\n", action);
    return false;
}
