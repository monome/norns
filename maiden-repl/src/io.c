#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nng/nng.h>
#include <nng/protocol/bus0/bus.h>
#include <nng/transport/ws/websocket.h>

#include "io.h"
#include "ui.h"

//------------------------------------------------------------------------------
//---- defines, types ----------------------------------------------------------

// max message size for nng recv
#define BUF_SIZE 4096

// per-channel state (socket connection and/or thread)
struct sock_io {
    nng_socket sock;
    nng_dialer dialer;
    pthread_t tid;
    bool has_thread; // ugh
};

//------------------------------------------------------------------------------
//---- function declarations ---------------------------------------------------

// loop to receive data from matron
static void *matron_rx_loop(void *p);
// loop to receive data from supercollider
static void *sc_rx_loop(void *p);
// ui input loop (dispatches typed commands to the active repl)
static void *tx_loop(void *x);

//------------------------------------------------------------------------------
//---- variables ---------------------------------------------------------------

// per-channel state (matron, supercollider, ui thread)
static struct sock_io sock_io[IO_COUNT];

// default websocket URLs (used when not overridden by argv)
static char *url_default[IO_COUNT] = {
    "ws4://127.0.0.1:5555/",
    "ws4://127.0.0.1:5556/",
    NULL,
};

// thread entry points for each channel
static void *(*loop_func[IO_COUNT])(void *) = {
    &matron_rx_loop,
    &sc_rx_loop,
    &tx_loop,
};

//------------------------------------------------------------------------------
//---- function definitions ----------------------------------------------------

// rewrite ws:// to ws4:// (or wss:// to wss4://) to force IPv4 via nng's
// built-in scheme suffix. returns NULL if the URL already specifies an
// address family or uses an unrecognized scheme. caller must free the result.
static char *make_ws4_url(const char *url) {
    const char *rest;
    const char *prefix;

    if (strncmp(url, "wss://", 6) == 0) {
        prefix = "wss4://";
        rest = url + 6;
    } else if (strncmp(url, "ws://", 5) == 0) {
        prefix = "ws4://";
        rest = url + 5;
    } else {
        return NULL;
    }

    size_t len = strlen(prefix) + strlen(rest) + 1;
    char *out = malloc(len);
    if (out) {
        snprintf(out, len, "%s%s", prefix, rest);
    }
    return out;
}

static void sock_io_init(struct sock_io *io, const char *url,
                         void *(*loop)(void *)) {
    if (url) {
        int rv;

        if ((rv = nng_bus0_open(&io->sock)) != 0) {
            ui_matron_line("socket open failed: ");
            ui_matron_line(nng_strerror(rv));
            ui_matron_line("\n");
        } else {
            // force IPv4 to avoid resolution failures with ws-wrapper
            char *ws4url = make_ws4_url(url);
            const char *connect_url = ws4url ? ws4url : url;

            if ((rv = nng_dialer_create(&io->dialer, io->sock, connect_url)) != 0 ||
                (rv = nng_dialer_set_bool(io->dialer, NNG_OPT_WS_SEND_TEXT, true)) != 0 ||
                (rv = nng_dialer_set_bool(io->dialer, NNG_OPT_WS_RECV_TEXT, true)) != 0 ||
                (rv = nng_dialer_start(io->dialer, 0)) != 0) {
                ui_matron_line("connection failed: (");
                ui_matron_line(url);
                ui_matron_line(") ");
                ui_matron_line(nng_strerror(rv));
                ui_matron_line("\n");
            }
            free(ws4url);
        }
    }

    io->has_thread = false;
    if (loop == NULL) {
        return;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (pthread_create(&io->tid, &attr, loop, io) == 0) {
        io->has_thread = true;
    } else {
        printf("error creating thread\n");
    }
    pthread_attr_destroy(&attr);
}

int io_init(int argc, char **argv) {
    for (int i = 0; i < IO_COUNT; i++) {
        char *url;
        if (argc > (i + 1)) {
            url = argv[i + 1];
        } else {
            url = url_default[i];
        }
        sock_io_init(&(sock_io[i]), url, loop_func[i]);
    }
    return 0;
}

int io_deinit(void) {
    for (int i = 0; i < IO_COUNT; i++) {
        if (i < IO_UI) {
            nng_close(sock_io[i].sock);
        }
        if (sock_io[i].has_thread && i != IO_UI) {
            pthread_join(sock_io[i].tid, NULL);
        }
    }
    return 0;
}

void io_send_line(int sockid, char *buf) {
    size_t sz = strlen(buf) + 2;
    char *line = calloc(sz, sizeof(char));
    if (!line) {
        return;
    }

    // sclang uses ESC (0x1b) as its evaluate delimiter, not newline.
    // matron uses newline (\n) as its line delimiter.
    char terminator = (sockid == IO_SC) ? '\x1b' : '\n';
    snprintf(line, sz, "%s", buf);
    line[sz - 2] = terminator;

    // exclude trailing null
    // a \0 on stdin confuses sclang
    nng_send(sock_io[sockid].sock, line, sz - 1, 0);
    free(line);
}

int io_loop(void) {
    for (int i = 0; i < IO_COUNT; i++) {
        if (sock_io[i].has_thread) {
            pthread_join(sock_io[i].tid, NULL);
        }
    }
    printf("\nfare well\n\n");
    return 0;
}

void *rx_loop(void *p, void (*ui_func)(const char *)) {
    struct sock_io *io = (struct sock_io *)p;
    char msg[BUF_SIZE];
    size_t nb;
    int rv;

    while (1) {
        nb = BUF_SIZE - 1;
        rv = nng_recv(io->sock, msg, &nb, 0);
        if (rv != 0) {
            break;
        }
        if (nb > 0) {
            msg[nb] = '\0';
            ui_func(msg);
        }
    }
    return NULL;
}

static void *matron_rx_loop(void *p) {
    return rx_loop(p, &ui_matron_line);
}

static void *sc_rx_loop(void *p) {
    return rx_loop(p, &ui_sc_line);
}

static void *tx_loop(void *x) {
    (void)x;
    ui_loop();
    io_deinit();
    return NULL;
}
