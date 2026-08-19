#include <assert.h>
#include <errno.h>
#include <grp.h>
#include <pthread.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <nng/nng.h>
#include <nng/protocol/pair1/pair.h>
#include <nng/transport/ipc/ipc.h>

#include "blockingconcurrentqueue.h"
#include "sidecar.h"
#include "sidecar_lines.h"
#include "sidecar_msg.h"
#include "sidecar_shell.h"

//---------------------------------
//--- common

static const char *url = "ipc:///tmp/norns-sidecar.ipc";

static const char *quit_cmd = "__quit__";

#define SIDECAR_CLIENT_RECV_MARGIN_MS 60000 // 1 min
#define SIDECAR_CLIENT_SENDTIMEO_MS 10000   // 10 s
#define SIDECAR_SERVER_SENDTIMEO_MS 10000   // 10 s

// bounds the error text captured from a failed launch
#define SIDECAR_DETACH_OUTPUT_BYTES 4096
// bounds what matron will buffer for one command, the server itself streams
#define SIDECAR_CMD_MAX_OUTPUT_BYTES (1024 * 1024)
// how long a dying command gets to run its cleanup traps before SIGKILL
#define SIDECAR_CMD_TERM_GRACE_MS 2000

#ifdef NORNS_TEST
// request allocators, tests point them at failing versions to hit the OOM paths
void *(*sidecar_test_calloc)(size_t, size_t) = calloc;
char *(*sidecar_test_strdup)(const char *) = strdup;
#define REQUEST_CALLOC sidecar_test_calloc
#define REQUEST_STRDUP sidecar_test_strdup
#else
#define REQUEST_CALLOC calloc
#define REQUEST_STRDUP strdup
#endif

static void sidecar_nng_error(const char *func, int rv) {
    fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
}

//---------------------------------
//--- frames

static nng_msg *frame_alloc(size_t len) {
    nng_msg *msg = NULL;
    int rv = nng_msg_alloc(&msg, len);
    if (rv != 0) {
        sidecar_nng_error("sidecar: frame alloc failed", rv);
        return NULL;
    }
    return msg;
}

static int frame_send(nng_socket sock, nng_msg *msg) {
    int rv = nng_sendmsg(sock, msg, 0);
    if (rv != 0) {
        nng_msg_free(msg);
    }
    return rv;
}

static nng_msg *frame_build_cmd(uint8_t req_id, uint32_t timeout_ms, const char *cmd) {
    size_t n = sidecar_msg_encode_cmd(NULL, 0, req_id, timeout_ms, cmd);
    nng_msg *msg = frame_alloc(n);
    if (msg != NULL) {
        sidecar_msg_encode_cmd((uint8_t *)nng_msg_body(msg), n, req_id, timeout_ms, cmd);
    }
    return msg;
}

static nng_msg *frame_build_detach(uint8_t req_id, const char *unit, const char *cmd) {
    size_t n = sidecar_msg_encode_detach(NULL, 0, req_id, unit, cmd);
    nng_msg *msg = frame_alloc(n);
    if (msg != NULL) {
        sidecar_msg_encode_detach((uint8_t *)nng_msg_body(msg), n, req_id, unit, cmd);
    }
    return msg;
}

static int frame_send_chunk(nng_socket sock, uint8_t req_id, const char *payload, size_t len) {
    size_t n = sidecar_msg_encode_chunk(NULL, 0, req_id, payload, len);
    nng_msg *msg = frame_alloc(n);
    if (msg == NULL) {
        return NNG_ENOMEM;
    }
    sidecar_msg_encode_chunk((uint8_t *)nng_msg_body(msg), n, req_id, payload, len);
    return frame_send(sock, msg);
}

static void frame_send_end(nng_socket sock, uint8_t req_id, bool signalled, uint8_t code) {
    size_t n = sidecar_msg_encode_end(NULL, 0, req_id, signalled, code);
    nng_msg *msg = frame_alloc(n);
    if (msg != NULL) {
        sidecar_msg_encode_end((uint8_t *)nng_msg_body(msg), n, req_id, signalled, code);
        frame_send(sock, msg);
    }
}

static void frame_send_error(nng_socket sock, uint8_t req_id, const char *text) {
    size_t n = sidecar_msg_encode_error(NULL, 0, req_id, text);
    nng_msg *msg = frame_alloc(n);
    if (msg != NULL) {
        sidecar_msg_encode_error((uint8_t *)nng_msg_body(msg), n, req_id, text);
        frame_send(sock, msg);
    }
}

//---------------------------------
//--- request queue

typedef enum {
    REQUEST_FIRST_REQUEST = 0,
    REQUEST_CMD,
    REQUEST_DETACH,
    REQUEST_QUIT,
} request_type_t;

struct request_common {
    request_type_t type;
};

struct request_cmd {
    struct request_common common;
    char *cmd;
    uint32_t timeout_ms;
    void *ctx;
    sidecar_chunk_cb_t on_chunk;
    sidecar_done_cb_t on_done;
};

struct request_detach {
    struct request_common common;
    char *cmd;
    char *unit;
    void *ctx;
    sidecar_done_cb_t on_done;
};

union request {
    request_type_t type;
    struct request_cmd cmd;
    struct request_detach detach;
};

static moodycamel::BlockingConcurrentQueue<union request *> requests(32);

static union request *request_new(request_type_t type) {
    union request *req = (request *)REQUEST_CALLOC(1, sizeof(union request));
    if (req != NULL) {
        req->type = type;
    }
    return req;
}

static void request_post(union request *req) {
    assert(req != NULL);
    requests.enqueue(req);
}

//---------------------------------
//--- server

static nng_msg *pending_request = NULL;

static int socket_recv_fd(nng_socket sock) {
    int fd = -1;
    int rv = nng_socket_get_int(sock, NNG_OPT_RECVFD, &fd);
    if (rv != 0) {
        sidecar_nng_error("sidecar: recv fd unavailable", rv);
        return -1;
    }
    return fd;
}

static bool request_arrived_during_cmd(nng_socket sock) {
    nng_msg *msg = NULL;
    if (nng_recvmsg(sock, &msg, NNG_FLAG_NONBLOCK) != 0) {
        return false;
    }

    pending_request = msg;
    return true;
}

struct stream_ctx {
    nng_socket sock;
    uint8_t req_id;
    int send_err;
    sidecar_lines_t lines;
};

static void stream_send_chunk(void *ctx, const char *line, size_t len) {
    struct stream_ctx *sc = (struct stream_ctx *)ctx;
    if (sc->send_err != 0) {
        return;
    }
    sc->send_err = frame_send_chunk(sc->sock, sc->req_id, line, len);
    if (sc->send_err != 0) {
        sidecar_nng_error("sidecar: stream send error", sc->send_err);
    }
}

static bool stream_on_output(void *ctx, const char *buf, size_t len) {
    struct stream_ctx *sc = (struct stream_ctx *)ctx;
    for (size_t i = 0; i < len && sc->send_err == 0; ++i) {
        sidecar_lines_push(&sc->lines, buf[i], sc, stream_send_chunk);
    }
    return sc->send_err == 0;
}

static bool stream_on_watch(void *ctx) {
    struct stream_ctx *sc = (struct stream_ctx *)ctx;
    return request_arrived_during_cmd(sc->sock);
}

static void sidecar_server_stream_cmd(nng_socket sock, const char *cmd, uint8_t req_id, uint32_t timeout_ms) {
    struct stream_ctx sc = {sock, req_id, 0, {}};
    sidecar_lines_init(&sc.lines);

    sidecar_shell_opts_t opts = {};
    opts.silence_timeout_ms = timeout_ms;
    opts.max_output_bytes = SIDECAR_CMD_MAX_OUTPUT_BYTES;
    opts.term_grace_ms = SIDECAR_CMD_TERM_GRACE_MS;
    opts.watch_fd = socket_recv_fd(sock);
    opts.ctx = &sc;
    opts.on_output = stream_on_output;
    opts.on_watch = stream_on_watch;

    sidecar_shell_result_t res;
    sidecar_shell_run(cmd, &opts, &res);

    switch (res.outcome) {
    case SIDECAR_SHELL_EXITED:
    case SIDECAR_SHELL_SIGNALLED:
        sidecar_lines_flush(&sc.lines, &sc, stream_send_chunk);
        if (sc.send_err != 0) {
            fprintf(stderr, "sidecar: command stopped (send failed mid-stream)\n");
            frame_send_error(sock, req_id, "send failed mid-stream");
            return;
        }
        frame_send_end(sock, req_id, res.outcome == SIDECAR_SHELL_SIGNALLED, (uint8_t)res.code);
        return;

    case SIDECAR_SHELL_ABANDONED:
        fprintf(stderr, "sidecar: command abandoned, norns stopped waiting for it\n");
        return;

    case SIDECAR_SHELL_QUIET:
        fprintf(stderr, "sidecar: command stopped (command went quiet)\n");
        frame_send_error(sock, req_id, "command went quiet");
        return;

    case SIDECAR_SHELL_CAPPED:
        fprintf(stderr, "sidecar: command stopped (output cap exceeded)\n");
        frame_send_error(sock, req_id, "output cap exceeded");
        return;

    case SIDECAR_SHELL_REFUSED:
        fprintf(stderr, "sidecar: command stopped (send failed mid-stream)\n");
        frame_send_error(sock, req_id, "send failed mid-stream");
        return;

    case SIDECAR_SHELL_FAILED:
        if (res.errnum != 0) {
            fprintf(stderr, "sidecar: %s: %s\n", res.err, strerror(res.errnum));
        } else {
            fprintf(stderr, "sidecar: %s\n", res.err);
        }
        frame_send_error(sock, req_id, res.err);
        return;
    }
}

static void sidecar_server_detach_cmd(nng_socket sock, const char *unit, const char *cmd, uint8_t req_id) {
    char unit_arg[128];
    char user_prop[96];
    char group_prop[96];

    if ((size_t)snprintf(unit_arg, sizeof(unit_arg), "--unit=%s", unit) >= sizeof(unit_arg)) {
        frame_send_error(sock, req_id, "unit name too long");
        return;
    }

    struct passwd *pw = getpwuid(getuid());
    if (pw != NULL) {
        snprintf(user_prop, sizeof(user_prop), "User=%s", pw->pw_name);
    } else {
        snprintf(user_prop, sizeof(user_prop), "User=%u", (unsigned)getuid());
    }
    struct group *gr = getgrgid(getgid());
    if (gr != NULL) {
        snprintf(group_prop, sizeof(group_prop), "Group=%s", gr->gr_name);
    } else {
        snprintf(group_prop, sizeof(group_prop), "Group=%u", (unsigned)getgid());
    }

    const char *argv[] = {
        "/usr/bin/sudo", "-n", "/usr/bin/env", "SYSTEMD_LOG_COLOR=0",
        "systemd-run", "--no-block", "--collect", unit_arg,
        "-p", user_prop, "-p", group_prop, "--service-type=oneshot",
        "/bin/sh", "-c", cmd, NULL};

    int fds[2];
    if (pipe(fds) != 0) {
        frame_send_error(sock, req_id, "pipe() failed");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        frame_send_error(sock, req_id, "fork() failed");
        return;
    }

    if (pid == 0) {
        dup2(fds[1], STDOUT_FILENO);
        dup2(fds[1], STDERR_FILENO);
        close(fds[0]);
        close(fds[1]);
        execv(argv[0], (char *const *)argv);
        fprintf(stderr, "exec of %s failed\n", argv[0]); // reaches the pipe
        _exit(127);
    }

    close(fds[1]);
    char output[SIDECAR_DETACH_OUTPUT_BYTES];
    size_t out_len = 0;
    for (;;) {
        char scrap[256];
        bool full = out_len >= sizeof(output) - 1;
        ssize_t n = read(fds[0],
                         full ? scrap : output + out_len,
                         full ? sizeof(scrap) : sizeof(output) - 1 - out_len);
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n <= 0) {
            break;
        }
        if (!full) {
            out_len += (size_t)n;
        }
    }
    close(fds[0]);
    output[out_len] = '\0';

    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited < 0) {
        frame_send_error(sock, req_id, "waitpid() failed");
        return;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        frame_send_end(sock, req_id, false, 0);
        return;
    }

    fprintf(stderr, "sidecar: detach failed for unit %s\n", unit);
    if (out_len == 0) {
        if (WIFEXITED(status)) {
            snprintf(output, sizeof(output), "systemd-run exit status %d", WEXITSTATUS(status));
        } else {
            snprintf(output, sizeof(output), "systemd-run killed by signal %d", WTERMSIG(status));
        }
    }
    frame_send_error(sock, req_id, output);
}

int sidecar_server_main(int sync_fd) {
    nng_socket sock;
    nng_listener listener;
    int rv;

    nng_ipc_register();

    if ((rv = nng_pair1_open(&sock)) != 0) {
        sidecar_nng_error("open socket failed", rv);
        close(sync_fd);
        return -1;
    }

    // remove stale ipc socket left behind after a hard kill.
    // safe because only one sidecar instance ever exists.
    unlink(url + strlen("ipc://"));

    if ((rv = nng_listen(sock, url, &listener, 0)) != 0) {
        nng_close(sock);
        sidecar_nng_error("binding socket failed", rv);
        close(sync_fd);
        return -1;
    }

    nng_socket_set_ms(sock, NNG_OPT_SENDTIMEO, SIDECAR_SERVER_SENDTIMEO_MS);

    // signal parent that IPC is bound and ready
    char b = 1;
    write(sync_fd, &b, 1);
    close(sync_fd);

    for (;;) {
        nng_msg *msg = NULL;

        if (pending_request != NULL) {
            msg = pending_request;
            pending_request = NULL;
        } else if ((rv = nng_recvmsg(sock, &msg, 0)) != 0) {
            sidecar_nng_error("recv error", rv);
            continue;
        }

        sidecar_frame_t frame;
        if (!sidecar_msg_parse(nng_msg_body(msg), nng_msg_len(msg), &frame)) {
            fprintf(stderr, "sidecar: malformed frame from client\n");
            nng_msg_free(msg);
            continue;
        }

        if (frame.type == SIDECAR_MSG_CMD) {
            uint8_t req_id = frame.req_id;
            uint32_t timeout_ms = frame.timeout_ms;
            char *cmd = strdup(frame.cmd);
            nng_msg_free(msg);

            if (cmd == NULL) {
                fprintf(stderr, "sidecar: strdup failed\n");
                frame_send_error(sock, req_id, "out of memory");
                continue;
            }

            if (strcmp(cmd, quit_cmd) == 0) {
                free(cmd);
                frame_send_end(sock, req_id, false, 0);
                break;
            }

            sidecar_server_stream_cmd(sock, cmd, req_id, timeout_ms);
            free(cmd);

        } else if (frame.type == SIDECAR_MSG_DETACH) {
            sidecar_server_detach_cmd(sock, frame.unit, frame.cmd, frame.req_id);
            nng_msg_free(msg);

        } else {
            fprintf(stderr, "sidecar: unexpected msg type 0x%02x from client\n", (unsigned)frame.type);
            nng_msg_free(msg);
        }
    }

    nng_close(sock);
    return 0;
}

//---------------------------------
//--- client

struct client_state {
    nng_socket sock;
    nng_dialer dialer;
    bool initialized;
};

static pthread_t client_thread;
static struct client_state cs;
static pthread_mutex_t run_cmd_lock = PTHREAD_MUTEX_INITIALIZER;
static union request *quit_request = NULL;

static void client_apply_timeouts() {
    nng_socket_set_ms(cs.sock, NNG_OPT_SENDTIMEO, SIDECAR_CLIENT_SENDTIMEO_MS);
}

static void client_apply_recv_timeout(uint32_t timeout_ms) {
    // the timeout applies to each recv, so every chunk resets it and the
    // client bounds silence between frames just like the server does
    const nng_duration wait = timeout_ms == 0 ? NNG_DURATION_INFINITE : (nng_duration)(timeout_ms + SIDECAR_CLIENT_RECV_MARGIN_MS);
    nng_socket_set_ms(cs.sock, NNG_OPT_RECVTIMEO, wait);
}

static void client_resync() {
    nng_close(cs.sock);
    cs.initialized = false;
    if (nng_pair1_open(&cs.sock) != 0) {
        fprintf(stderr, "sidecar: resync open failed\n");
        return;
    }
    client_apply_timeouts();
    if (nng_dial(cs.sock, url, &cs.dialer, 0) != 0) {
        fprintf(stderr, "sidecar: resync dial failed\n");
        return;
    }
    cs.initialized = true;
}

//---------------------------------
//--- command transaction

// serialize interaction with the sidecar so sync and async command invocation
// is not interleaved
static void transport_lock() {
    if (pthread_mutex_trylock(&run_cmd_lock) == 0) {
        return;
    }
    fprintf(stderr, "sidecar: a command is still running, waiting for the transport\n");
    pthread_mutex_lock(&run_cmd_lock);
}

static uint8_t next_req_id = 0;

static void transaction_abort(const char *cmd, void *ctx, sidecar_done_cb_t on_done, const char *err) {
    client_resync();
    pthread_mutex_unlock(&run_cmd_lock);

    const sidecar_status_t status = {SIDECAR_TRANSPORT_FAILED, false, 0, err};
    on_done(cmd, ctx, &status);
}

static void transaction_send_recv(nng_msg *req, uint8_t req_id, const char *cmd, void *ctx, sidecar_chunk_cb_t on_chunk, sidecar_done_cb_t on_done) {
    int rv;

    if ((rv = frame_send(cs.sock, req)) != 0) {
        sidecar_nng_error("stream: send error", rv);
        transaction_abort(cmd, ctx, on_done, "sidecar send failed");
        return;
    }

    for (;;) {
        nng_msg *msg = NULL;
        if ((rv = nng_recvmsg(cs.sock, &msg, 0)) != 0) {
            sidecar_nng_error("stream: recv error", rv);
            transaction_abort(cmd, ctx, on_done, "sidecar did not answer");
            return;
        }

        sidecar_frame_t frame;
        if (!sidecar_msg_parse(nng_msg_body(msg), nng_msg_len(msg), &frame)) {
            fprintf(stderr, "sidecar: malformed frame from sidecar\n");
            nng_msg_free(msg);
            continue;
        }

        if (frame.req_id != req_id) {
            nng_msg_free(msg);
            continue;
        }

        if (frame.type == SIDECAR_MSG_CHUNK) {
            if (on_chunk != NULL) {
                on_chunk(ctx, frame.payload, frame.payload_len);
            }
            nng_msg_free(msg);

        } else if (frame.type == SIDECAR_MSG_END) {
            sidecar_status_t status = {SIDECAR_OK, frame.signalled, frame.code, NULL};
            char exit_text[48];
            if (frame.signalled || frame.code != 0) {
                status.result = SIDECAR_CMD_FAILED;
                snprintf(exit_text, sizeof(exit_text), frame.signalled ? "killed by signal %d" : "exit status %d", frame.code);
                status.err = exit_text;
            }
            nng_msg_free(msg);
            pthread_mutex_unlock(&run_cmd_lock);
            on_done(cmd, ctx, &status);
            return;

        } else if (frame.type == SIDECAR_MSG_ERROR) {
            char err_text[SIDECAR_DETACH_OUTPUT_BYTES];
            snprintf(err_text, sizeof(err_text), "%s", frame.payload_len > 0 ? frame.payload : "unknown error");
            nng_msg_free(msg);
            const sidecar_status_t status = {SIDECAR_CMD_FAILED, false, 0, err_text};
            pthread_mutex_unlock(&run_cmd_lock);
            on_done(cmd, ctx, &status);
            return;

        } else {
            nng_msg_free(msg);
        }
    }
}

static void transaction_out_of_memory(const char *cmd, void *ctx, sidecar_done_cb_t on_done) {
    pthread_mutex_unlock(&run_cmd_lock);
    const sidecar_status_t status = {SIDECAR_TRANSPORT_FAILED, false, 0, "out of memory"};
    on_done(cmd, ctx, &status);
}

static void cmd_transaction(const char *cmd, uint32_t timeout_ms, void *ctx, sidecar_chunk_cb_t on_chunk, sidecar_done_cb_t on_done) {
    transport_lock();
    uint8_t req_id = ++next_req_id;

    nng_msg *req = frame_build_cmd(req_id, timeout_ms, cmd);
    if (req == NULL) {
        fprintf(stderr, "sidecar: failed to build CMD frame\n");
        transaction_out_of_memory(cmd, ctx, on_done);
        return;
    }

    client_apply_recv_timeout(timeout_ms);
    transaction_send_recv(req, req_id, cmd, ctx, on_chunk, on_done);
}

static void detach_transaction(const char *cmd, const char *unit, void *ctx, sidecar_done_cb_t on_done) {
    transport_lock();
    uint8_t req_id = ++next_req_id;

    nng_msg *req = frame_build_detach(req_id, unit, cmd);
    if (req == NULL) {
        fprintf(stderr, "sidecar: failed to build DETACH frame\n");
        transaction_out_of_memory(cmd, ctx, on_done);
        return;
    }

    client_apply_recv_timeout(SIDECAR_CMD_TIMEOUT_DEFAULT_MS);
    transaction_send_recv(req, req_id, cmd, ctx, NULL, on_done);
}

static void handle_request(union request *req) {
    switch (req->type) {
    case REQUEST_CMD:
        cmd_transaction(req->cmd.cmd, req->cmd.timeout_ms, req->cmd.ctx, req->cmd.on_chunk, req->cmd.on_done);
        free(req->cmd.cmd);
        break;
    case REQUEST_DETACH:
        detach_transaction(req->detach.cmd, req->detach.unit, req->detach.ctx, req->detach.on_done);
        free(req->detach.cmd);
        free(req->detach.unit);
        break;
    default:
        fprintf(stderr, "sidecar: unhandled request type %d\n", req->type);
        break;
    }
}

static void *sidecar_client_loop(void *) {
    union request *req = nullptr;

    for (;;) {
        requests.wait_dequeue(req);
        if (req->type == REQUEST_QUIT) {
            free(req);
            break;
        }
        handle_request(req);
        free(req);
    }

    nng_close(cs.sock);
    return NULL;
}

//
// external
//

int sidecar_client_init() {
    int rv;

    nng_ipc_register();

    cs.initialized = false;

    if ((rv = nng_pair1_open(&cs.sock)) != 0) {
        sidecar_nng_error("sidecar: open socket failed", rv);
        return -1;
    }

    client_apply_timeouts();

    if ((rv = nng_dial(cs.sock, url, &cs.dialer, 0)) != 0) {
        sidecar_nng_error("sidecar: establishing connection failed", rv);
        return -1;
    }

    quit_request = request_new(REQUEST_QUIT);
    if (quit_request == NULL) {
        fprintf(stderr, "sidecar: unable to allocate quit request\n");
        return -1;
    }

    cs.initialized = true;

    if (pthread_create(&client_thread, NULL, sidecar_client_loop, NULL)) {
        fprintf(stderr, "sidecar: unable to create client thread\n");
        return -1;
    }
    pthread_setname_np(client_thread, "client_loop");

    return 0;
}

static void noop_done_cb(const char *cmd, void *ctx, const sidecar_status_t *status) {
    (void)cmd;
    (void)ctx;
    (void)status;
}

void sidecar_client_cleanup() {
    if (!cs.initialized) {
        return;
    }

    // Tell the sidecar server to exit
    cmd_transaction(quit_cmd, SIDECAR_CMD_TIMEOUT_DEFAULT_MS, NULL, NULL, noop_done_cb);

    // Tell the client thread to exit
    request_post(quit_request);
    quit_request = NULL;

    // Wait for the background thread to finish
    pthread_join(client_thread, NULL);

    cs.initialized = false;
}

void sidecar_client_cmd(const char *cmd, uint32_t timeout_ms, void *ctx, sidecar_chunk_cb_t on_chunk, sidecar_done_cb_t on_done) {
    cmd_transaction(cmd, timeout_ms, ctx, on_chunk, on_done);
}

bool sidecar_client_cmd_async(const char *cmd, uint32_t timeout_ms, void *ctx, sidecar_chunk_cb_t on_chunk, sidecar_done_cb_t on_done) {
    union request *req = request_new(REQUEST_CMD);
    if (req == NULL) {
        return false;
    }
    req->cmd.cmd = REQUEST_STRDUP(cmd);
    if (req->cmd.cmd == NULL) {
        free(req);
        return false;
    }
    req->cmd.timeout_ms = timeout_ms;
    req->cmd.ctx = ctx;
    req->cmd.on_chunk = on_chunk;
    req->cmd.on_done = on_done;
    request_post(req);
    return true;
}

bool sidecar_client_detach_async(const char *cmd, const char *unit, void *ctx, sidecar_done_cb_t on_done) {
    union request *req = request_new(REQUEST_DETACH);
    if (req == NULL) {
        return false;
    }
    req->detach.cmd = REQUEST_STRDUP(cmd);
    req->detach.unit = REQUEST_STRDUP(unit);
    if (req->detach.cmd == NULL || req->detach.unit == NULL) {
        free(req->detach.cmd);
        free(req->detach.unit);
        free(req);
        return false;
    }
    req->detach.ctx = ctx;
    req->detach.on_done = on_done;
    request_post(req);
    return true;
}
