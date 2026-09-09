#ifndef _NORNS_SIDECAR_H_
#define _NORNS_SIDECAR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// timeouts bound the longest quiet stretch between output, not total
// duration, so a command that keeps producing output is never cut off
#define SIDECAR_CMD_TIMEOUT_DEFAULT_MS 240000 // 4 min
#define SIDECAR_CMD_TIMEOUT_NONE 0

// main loop of sidecar server process. sync_fd is a file descriptor that will be
// written to when the server is ready to accept connections.
int sidecar_server_main(int sync_fd);

// initialize sidecar client IPC connections
int sidecar_client_init();

// cleanup sidecar client IPC connections
void sidecar_client_cleanup();

typedef enum {
    SIDECAR_OK = 0,           // the command ran and exited zero
    SIDECAR_CMD_FAILED,       // the command exited non-zero, or never ran at all
    SIDECAR_TRANSPORT_FAILED, // the sidecar never answered
} sidecar_result_t;

typedef struct {
    sidecar_result_t result;
    bool signalled;  // code is a signal number, not an exit status
    int code;        // exit status, or the signal that killed it
    const char *err; // failure text, NULL when result is SIDECAR_OK
} sidecar_status_t;

typedef void (*sidecar_chunk_cb_t)(void *ctx, const char *buf, size_t size);
typedef void (*sidecar_done_cb_t)(const char *cmd, void *ctx, const sidecar_status_t *status);

// request a command to be executed synchronously by the sidecar server
// calls the callbacks on the invoking thread, the outcome arrives via status
void sidecar_client_cmd(const char *cmd, uint32_t timeout_ms, void *ctx, sidecar_chunk_cb_t on_chunk, sidecar_done_cb_t on_done);

// request a command to be executed asynchronously by the sidecar server
// returns false when the request could not be allocated, else enqueues it
// and calls the callbacks from a background thread as output arrives
bool sidecar_client_cmd_async(const char *cmd, uint32_t timeout_ms, void *ctx, sidecar_chunk_cb_t on_chunk, sidecar_done_cb_t on_done);

// request a command be launched detached in a transient systemd unit, so it
// outlives norns. its output goes to the journal, so there is no on_chunk.
// returns false when the request could not be allocated
bool sidecar_client_detach_async(const char *cmd, const char *unit, void *ctx, sidecar_done_cb_t on_done);

#ifdef NORNS_TEST
extern void *(*sidecar_test_calloc)(size_t, size_t);
extern char *(*sidecar_test_strdup)(const char *);
#endif

#endif // _NORNS_SIDECAR_H_
