#ifndef _NORNS_SIDECAR_LINES_H_
#define _NORNS_SIDECAR_LINES_H_

#include <stdbool.h>
#include <stddef.h>

// splits output on '\n' and '\r' (so progress bars aren't one giant line),
// keeping every byte, so segments concatenate back to the exact output.

#define SIDECAR_LINE_BYTES 4096

typedef void (*sidecar_lines_cb_t)(void *ctx, const char *line, size_t len);

typedef struct {
    char buf[SIDECAR_LINE_BYTES];
    size_t len;
    bool cr_pending;
} sidecar_lines_t;

void sidecar_lines_init(sidecar_lines_t *s);
void sidecar_lines_push(sidecar_lines_t *s, char c, void *ctx, sidecar_lines_cb_t on_line);
void sidecar_lines_flush(sidecar_lines_t *s, void *ctx, sidecar_lines_cb_t on_line);

#endif // _NORNS_SIDECAR_LINES_H_
