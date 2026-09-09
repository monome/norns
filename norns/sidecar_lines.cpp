#include <string.h>

#include "sidecar_lines.h"

void sidecar_lines_init(sidecar_lines_t *s) {
    memset(s, 0, sizeof(*s));
}

static void lines_flush_segment(sidecar_lines_t *s, void *ctx, sidecar_lines_cb_t on_line) {
    if (s->len == 0) {
        return;
    }
    on_line(ctx, s->buf, s->len);
    s->len = 0;
}

static void lines_append(sidecar_lines_t *s, char c, void *ctx, sidecar_lines_cb_t on_line) {
    if (s->len == SIDECAR_LINE_BYTES) {
        lines_flush_segment(s, ctx, on_line);
    }
    s->buf[s->len++] = c;
}

void sidecar_lines_push(sidecar_lines_t *s, char c, void *ctx, sidecar_lines_cb_t on_line) {
    if (s->cr_pending) {
        s->cr_pending = false;
        if (c != '\n') {
            lines_flush_segment(s, ctx, on_line);
        }
    }

    if (c == '\r') {
        lines_append(s, '\r', ctx, on_line);
        s->cr_pending = true;
        return;
    }

    if (c == '\n') {
        lines_append(s, '\n', ctx, on_line);
        lines_flush_segment(s, ctx, on_line);
        return;
    }

    lines_append(s, c, ctx, on_line);
}

void sidecar_lines_flush(sidecar_lines_t *s, void *ctx, sidecar_lines_cb_t on_line) {
    s->cr_pending = false;
    lines_flush_segment(s, ctx, on_line);
}
