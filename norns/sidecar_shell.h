#ifndef _NORNS_SIDECAR_SHELL_H_
#define _NORNS_SIDECAR_SHELL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// runs one shell command under supervision and streams its output as it
// arrives. transport-free, the sidecar server wires the callbacks to NNG.

typedef enum {
    SIDECAR_SHELL_EXITED,    // ran to completion, code holds the exit status
    SIDECAR_SHELL_SIGNALLED, // died on a signal, code holds the signal
    SIDECAR_SHELL_QUIET,     // produced no output within the silence budget
    SIDECAR_SHELL_CAPPED,    // produced more output than max_output_bytes
    SIDECAR_SHELL_REFUSED,   // on_output returned false
    SIDECAR_SHELL_ABANDONED, // on_watch said the caller stopped waiting
    SIDECAR_SHELL_FAILED,    // never ran or supervision broke, err says why
} sidecar_shell_outcome_t;

typedef struct {
    sidecar_shell_outcome_t outcome;
    int code;        // exit status or signal number
    const char *err; // static text for SIDECAR_SHELL_FAILED, else NULL
    int errnum;      // errno at the failure for SIDECAR_SHELL_FAILED, else 0
} sidecar_shell_result_t;

typedef struct {
    // longest quiet stretch allowed between output, zero for no ceiling
    uint32_t silence_timeout_ms;
    // output cap in bytes, zero for no cap
    size_t max_output_bytes;
    // how long SIGTERM gets before SIGKILL, zero kills at once
    uint32_t term_grace_ms;
    int watch_fd;
    void *ctx;
    bool (*on_output)(void *ctx, const char *buf, size_t len);
    bool (*on_watch)(void *ctx);
} sidecar_shell_opts_t;

// runs cmd via /bin/sh with stdin from /dev/null and stdout and stderr
// merged into the output stream, blocks until the command is over
void sidecar_shell_run(const char *cmd, const sidecar_shell_opts_t *opts, sidecar_shell_result_t *result);

#endif // _NORNS_SIDECAR_SHELL_H_
