#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "screen.h"
#include "screen_events.h"
#include "screen_events_pr.h"

#define SCREEN_Q_SIZE_INIT 1024
#define SCREEN_Q_MASK_INIT (SCREEN_Q_SIZE_INIT - 1)
#define SCREEN_Q_SIZE_MAX  8192

static size_t screen_q_size = SCREEN_Q_SIZE_INIT;
static size_t screen_q_mask = SCREEN_Q_MASK_INIT;

static struct screen_event_data * screen_q;
static size_t screen_q_wr = 0;
static size_t screen_q_rd = 0;

static void *screen_event_loop(void *);

static void screen_event_data_push(struct screen_event_data *src);
static void screen_event_data_pop(struct screen_event_data *dst);
static void screen_event_data_init(struct screen_event_data *ev);
static void screen_event_data_free(struct screen_event_data *ev);
static void screen_event_data_move(struct screen_event_data *dst, struct screen_event_data *src);
static void handle_screen_event(struct screen_event_data *ev);

static pthread_t screen_event_thread;
static pthread_mutex_t screen_q_lock;
static pthread_cond_t screen_q_nonempty;

// clear out the Q, then populate it with a visible white flash
static void screen_events_emergency_clear();

// double the size of the Q after it is cleared in emergency
static void screen_events_grow_queue();

// halve the size of the Q after if it has been running fine
// after switching scripts
static void screen_events_shrink_queue();

void screen_events_init() {
    screen_q = calloc(screen_q_size, sizeof(struct screen_event_data));
    if (screen_q == NULL) {
        fprintf(stderr, "SCREEN: error allocating queue\n");
    }

    if (pthread_mutex_init(&screen_q_lock, NULL) != 0 ){
        fprintf(stderr, "SCREEN: error creating lock\n");
    }

    if (pthread_cond_init(&screen_q_nonempty, NULL) != 0 ){
        fprintf(stderr, "SCREEN: error creating condition\n");
    }

    if (pthread_create(&screen_event_thread, NULL, screen_event_loop, 0) != 0) {
        fprintf(stderr, "SCREEN: error creating thread\n");
    }
}

void screen_events_reset() {
    pthread_mutex_lock(&screen_q_lock);
    screen_events_shrink_queue();
    pthread_mutex_unlock(&screen_q_lock);
}

void screen_event_data_init(struct screen_event_data *ev) {
    ev->type = SCREEN_EVENT_NONE;
    ev->buf = NULL;
}

void screen_event_data_free(struct screen_event_data *ev) {
    if (ev->buf != NULL) {
        free(ev->buf);
    }
    screen_event_data_init(ev);
}

static void screen_event_data_move(struct screen_event_data *dst, struct screen_event_data *src) {
    assert(dst->buf == NULL && dst->type == SCREEN_EVENT_NONE);
    *dst = *src;
    screen_event_data_init(src);
}

// call from any thread; locks Q (blocking)
void screen_event_data_push(struct screen_event_data *src) {
    pthread_mutex_lock(&screen_q_lock);
    struct screen_event_data *dst = &(screen_q[screen_q_wr]);
    screen_event_data_move(dst, src);
    screen_q_wr = (screen_q_wr + 1) & screen_q_mask;
    // if these indices become equal here, we've filled the queue
    if (screen_q_wr == screen_q_rd) {
        fprintf(stderr, "warning: screen event Q full!\n");
        screen_events_emergency_clear();
        screen_events_grow_queue();
    }
    pthread_cond_signal(&screen_q_nonempty);
    pthread_mutex_unlock(&screen_q_lock);
}

// call with Q locked from screen handler thread
void screen_event_data_pop(struct screen_event_data *dst) {
    struct screen_event_data *src = &screen_q[screen_q_rd];
    screen_event_data_move(dst, src);
    screen_q_rd = (screen_q_rd + 1) & screen_q_mask;
}

void *screen_event_loop(void *x) {
    (void)x;
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    while (1) {
        pthread_mutex_lock(&screen_q_lock);
        while (screen_q_rd == screen_q_wr) {
            pthread_cond_wait(&screen_q_nonempty, &screen_q_lock);
        }
        assert(screen_q_rd != screen_q_wr);
        screen_event_data_pop(&ev);
        pthread_mutex_unlock(&screen_q_lock);
        handle_screen_event(&ev);
    }
}

void handle_screen_event(struct screen_event_data *ev) {
    assert(ev->type != SCREEN_EVENT_NONE);
    switch (ev->type) {
    case SCREEN_EVENT_UPDATE:
        screen_update();
        break;
    case SCREEN_EVENT_SAVE:
        screen_save();
        break;
    case SCREEN_EVENT_RESTORE:
        screen_restore();
        break;
    case SCREEN_EVENT_FONT_FACE:
        screen_font_face(ev->payload.i.i1);
        break;
    case SCREEN_EVENT_FONT_SIZE:
        screen_font_size(ev->payload.d.d1);
        break;
    case SCREEN_EVENT_AA:
        screen_aa(ev->payload.i.i1);
        break;
    case SCREEN_EVENT_LEVEL:
        screen_level(ev->payload.i.i1);
        break;
    case SCREEN_EVENT_LINE_WIDTH:
        screen_line_width(ev->payload.d.d1);
        break;
    case SCREEN_EVENT_LINE_CAP:
        screen_line_cap(ev->buf);
        break;
    case SCREEN_EVENT_LINE_JOIN:
        screen_line_join(ev->buf);
        break;
    case SCREEN_EVENT_MITER_LIMIT:
        screen_miter_limit(ev->payload.d.d1);
        break;
    case SCREEN_EVENT_MOVE:
        screen_move(ev->payload.d.d1, ev->payload.d.d2);
        break;
    case SCREEN_EVENT_LINE:
        screen_line(ev->payload.d.d1, ev->payload.d.d2);
        break;
    case SCREEN_EVENT_MOVE_REL:
        screen_move_rel(ev->payload.d.d1, ev->payload.d.d2);
        break;
    case SCREEN_EVENT_LINE_REL:
        screen_line_rel(ev->payload.d.d1, ev->payload.d.d2);
        break;
    case SCREEN_EVENT_CURVE:
        screen_curve(ev->payload.d.d1, ev->payload.d.d2, ev->payload.d.d3, ev->payload.d.d4, ev->payload.d.d5,
                     ev->payload.d.d6);
        break;
    case SCREEN_EVENT_CURVE_REL:
        screen_curve_rel(ev->payload.d.d1, ev->payload.d.d2, ev->payload.d.d3, ev->payload.d.d4, ev->payload.d.d5,
                         ev->payload.d.d6);
        break;
    case SCREEN_EVENT_ARC:
        screen_arc(ev->payload.d.d1, ev->payload.d.d2, ev->payload.d.d3, ev->payload.d.d4, ev->payload.d.d5);
        break;
    case SCREEN_EVENT_RECT:
        screen_rect(ev->payload.d.d1, ev->payload.d.d2, ev->payload.d.d3, ev->payload.d.d4);
        break;
    case SCREEN_EVENT_STROKE:
        screen_stroke();
        break;
    case SCREEN_EVENT_FILL:
        screen_fill();
        break;
    case SCREEN_EVENT_TEXT:
        screen_text(ev->buf);
        break;
    case SCREEN_EVENT_TEXT_RIGHT:
        screen_text_right(ev->buf);
        break;
    case SCREEN_EVENT_TEXT_CENTER:
        screen_text_center(ev->buf);
        break;
    case SCREEN_EVENT_TEXT_EXTENTS:
        screen_text_extents(ev->buf);
        break;
    case SCREEN_EVENT_TEXT_TRIM:
        screen_text_trim(ev->buf, ev->payload.d.d1);
        break;
    case SCREEN_EVENT_CLEAR:
        screen_clear();
        break;
    case SCREEN_EVENT_CLOSE_PATH:
        screen_close_path();
        break;
    case SCREEN_EVENT_EXPORT_PNG:
        screen_export_png(ev->buf);
        break;
    case SCREEN_EVENT_DISPLAY_PNG:
        screen_display_png(ev->buf, ev->payload.bd.d1, ev->payload.bd.d2);
        break;
    case SCREEN_EVENT_DISPLAY_SURFACE:
        screen_surface_display(ev->buf, ev->payload.bd.d1, ev->payload.bd.d2);
        ev->buf = NULL; // Didn't make a copy of the pointer, nullify to avoid double free.
        break;
    case SCREEN_EVENT_DISPLAY_SURFACE_REGION:
        screen_surface_display_region(ev->buf, ev->payload.d.d1, ev->payload.d.d2, ev->payload.d.d3, ev->payload.d.d4, ev->payload.d.d5, ev->payload.d.d6);
        ev->buf = NULL; // Didn't make a copy of the pointer, nullify to avoid double free.
        break;
    case SCREEN_EVENT_ROTATE:
        screen_rotate(ev->payload.d.d1);
        break;
    case SCREEN_EVENT_TRANSLATE:
        screen_translate(ev->payload.d.d1, ev->payload.d.d2);
        break;
    case SCREEN_EVENT_SET_OPERATOR:
        screen_set_operator(ev->payload.i.i1);
        break;
    case SCREEN_EVENT_POKE:
        screen_poke(ev->payload.bi.i1, ev->payload.bi.i2, ev->payload.bi.i3, ev->payload.bi.i4, ev->buf);
        break;
    case SCREEN_EVENT_PEEK:
        screen_peek(ev->payload.bi.i1, ev->payload.bi.i2, ev->payload.bi.i3, ev->payload.bi.i4);
        break;
    case SCREEN_EVENT_CURRENT_POINT:
        screen_current_point();
        break;
    case SCREEN_EVENT_GAMMA:
        screen_gamma(ev->payload.d.d1);
        break;
    case SCREEN_EVENT_BRIGHTNESS:
        screen_brightness(ev->payload.i.i1);
        break;
    case SCREEN_EVENT_CONTRAST:
        screen_contrast(ev->payload.i.i1);
        break;
    case SCREEN_EVENT_INVERT:
        screen_invert(ev->payload.i.i1);
        break;
    default:;
        ;
    }

    screen_event_data_free(ev);
}

//--------------------
//-- these functions allocate buffer memory as needed

// helper for null-terminated string arguments
static inline void screen_event_copy_string(struct screen_event_data *ev, const char *s) {
    size_t nb = strlen(s) + 1;
    ev->buf = malloc(nb);
    memcpy(ev->buf, s, nb);
    ev->payload.b.nb = nb;
}

void screen_event_update(void) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_UPDATE;
    screen_event_data_push(&ev);
}

void screen_event_save(void) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_SAVE;
    screen_event_data_push(&ev);
}

void screen_event_restore(void) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_RESTORE;
    screen_event_data_push(&ev);
}

void screen_event_font_face(int i) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_FONT_FACE;
    ev.payload.i.i1 = i;
    screen_event_data_push(&ev);
}

void screen_event_font_size(double z) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_FONT_SIZE;
    ev.payload.d.d1 = z;
    screen_event_data_push(&ev);
}

void screen_event_aa(int z) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_AA;
    ev.payload.i.i1 = z;
    screen_event_data_push(&ev);
}

void screen_event_level(int z) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_LEVEL;
    ev.payload.i.i1 = z;
    screen_event_data_push(&ev);
}

void screen_event_line_width(double w) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_LINE_WIDTH;
    ev.payload.d.d1 = w;
    screen_event_data_push(&ev);
}

void screen_event_line_cap(const char *s) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_LINE_CAP;
    screen_event_copy_string(&ev, s);
    screen_event_data_push(&ev);
}

void screen_event_line_join(const char *s) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_LINE_JOIN;
    screen_event_copy_string(&ev, s);
    screen_event_data_push(&ev);
}

void screen_event_miter_limit(double limit) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_MITER_LIMIT;
    ev.payload.d.d1 = limit;
    screen_event_data_push(&ev);
}

void screen_event_move(double x, double y) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_MOVE;
    ev.payload.d.d1 = x;
    ev.payload.d.d2 = y;
    screen_event_data_push(&ev);
}

void screen_event_line(double x, double y) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_LINE;
    ev.payload.d.d1 = x;
    ev.payload.d.d2 = y;
    screen_event_data_push(&ev);
}

void screen_event_move_rel(double x, double y) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_MOVE_REL;
    ev.payload.d.d1 = x;
    ev.payload.d.d2 = y;
    screen_event_data_push(&ev);
}

void screen_event_line_rel(double x, double y) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_LINE_REL;
    ev.payload.d.d1 = x;
    ev.payload.d.d2 = y;
    screen_event_data_push(&ev);
}

void screen_event_curve(double x1, double y1, double x2, double y2, double x3, double y3) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_CURVE;
    ev.payload.d.d1 = x1;
    ev.payload.d.d2 = y1;
    ev.payload.d.d3 = x2;
    ev.payload.d.d4 = y2;
    ev.payload.d.d5 = x3;
    ev.payload.d.d6 = y3;
    screen_event_data_push(&ev);
}

void screen_event_curve_rel(double dx1, double dy1, double dx2, double dy2, double dx3, double dy3) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_CURVE_REL;
    ev.payload.d.d1 = dx1;
    ev.payload.d.d2 = dy1;
    ev.payload.d.d3 = dx2;
    ev.payload.d.d4 = dy2;
    ev.payload.d.d5 = dx3;
    ev.payload.d.d6 = dy3;
    screen_event_data_push(&ev);
}

void screen_event_arc(double x, double y, double r, double a1, double a2) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_ARC;
    ev.payload.d.d1 = x;
    ev.payload.d.d2 = y;
    ev.payload.d.d3 = r;
    ev.payload.d.d4 = a1;
    ev.payload.d.d5 = a2;
    screen_event_data_push(&ev);
}

void screen_event_rect(double x, double y, double w, double h) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_RECT;
    ev.payload.d.d1 = x;
    ev.payload.d.d2 = y;
    ev.payload.d.d3 = w;
    ev.payload.d.d4 = h;
    screen_event_data_push(&ev);
}

void screen_event_stroke(void) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_STROKE;
    screen_event_data_push(&ev);
}

void screen_event_fill(void) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_FILL;
    screen_event_data_push(&ev);
}

void screen_event_text(const char *s) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_TEXT;
    screen_event_copy_string(&ev, s);
    screen_event_data_push(&ev);
}

void screen_event_text_right(const char *s) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_TEXT_RIGHT;
    screen_event_copy_string(&ev, s);
    screen_event_data_push(&ev);
}

void screen_event_text_center(const char *s) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_TEXT_CENTER;
    screen_event_copy_string(&ev, s);
    screen_event_data_push(&ev);
}

void screen_event_text_extents(const char *s) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_TEXT_EXTENTS;
    screen_event_copy_string(&ev, s);
    screen_event_data_push(&ev);
}

void screen_event_text_trim(const char *s, double w) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_TEXT_TRIM;
    screen_event_copy_string(&ev, s);
    ev.payload.d.d1 = w;
    screen_event_data_push(&ev);
}

void screen_event_clear(void) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_CLEAR;
    screen_event_data_push(&ev);
}

void screen_event_close_path(void) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_CLOSE_PATH;
    screen_event_data_push(&ev);
}

void screen_event_export_png(const char *s) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_EXPORT_PNG;
    screen_event_copy_string(&ev, s);
    screen_event_data_push(&ev);
}

void screen_event_display_png(const char *s, double x, double y) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_DISPLAY_PNG;
    screen_event_copy_string(&ev, s);
    ev.payload.bd.d1 = x;
    ev.payload.bd.d2 = y;
    screen_event_data_push(&ev);
}

void screen_event_display_surface(void* surface, double x, double y) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_DISPLAY_SURFACE;
    ev.buf = surface;
    ev.payload.bd.d1 = x;
    ev.payload.bd.d2 = y;
    screen_event_data_push(&ev);
}

void screen_event_display_surface_region(void* surface, double left, double top, double width, double height, double x, double y) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_DISPLAY_SURFACE_REGION;
    ev.buf = surface;
    ev.payload.d.d1 = left;
    ev.payload.d.d2 = top;
    ev.payload.d.d3 = width;
    ev.payload.d.d4 = height;
    ev.payload.d.d5 = x;
    ev.payload.d.d6 = y;
    screen_event_data_push(&ev);
}

void screen_event_peek(int x, int y, int w, int h) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_PEEK;
    ev.payload.bi.i1 = x;
    ev.payload.bi.i2 = y;
    ev.payload.bi.i3 = w;
    ev.payload.bi.i4 = h;
    screen_event_data_push(&ev);
}

void screen_event_poke(int x, int y, int w, int h, unsigned char *buf) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_POKE;
    size_t nb = w * h;
    ev.payload.bd.nb = nb;
    ev.buf = malloc(nb);
    memcpy(ev.buf, buf, nb);
    ev.payload.bi.i1 = x;
    ev.payload.bi.i2 = y;
    ev.payload.bi.i3 = w;
    ev.payload.bi.i4 = h;
    screen_event_data_push(&ev);
}

void screen_event_rotate(double r) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_ROTATE;
    ev.payload.d.d1 = r;
    screen_event_data_push(&ev);
}

void screen_event_translate(double x, double y) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_TRANSLATE;
    ev.payload.d.d1 = x;
    ev.payload.d.d2 = y;
    screen_event_data_push(&ev);
}

void screen_event_set_operator(int i) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_SET_OPERATOR;
    ev.payload.i.i1 = i;
    screen_event_data_push(&ev);
}

void screen_event_current_point() {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_CURRENT_POINT;
    screen_event_data_push(&ev);
}

void screen_event_gamma(double g) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_GAMMA;
    ev.payload.d.d1 = g;
    screen_event_data_push(&ev);
}

void screen_event_brightness(int b) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_BRIGHTNESS;
    ev.payload.i.i1 = b;
    screen_event_data_push(&ev);
}

void screen_event_contrast(int c) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_CONTRAST;
    ev.payload.i.i1 = c;
    screen_event_data_push(&ev);
}

void screen_event_invert(int i) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_INVERT;
    ev.payload.i.i1 = i;
    screen_event_data_push(&ev);
}

// assumption: Q is locked
void screen_events_emergency_clear() {
    struct screen_event_data *ev;
    for (size_t i = 0; i < screen_q_size; ++i) {
        ev = &screen_q[i];
        screen_event_data_free(ev);
    }
    screen_q[0].type = SCREEN_EVENT_CLEAR;
    screen_q[1].type = SCREEN_EVENT_LEVEL;
    screen_q[1].payload.i.i1 = 15;
    screen_q[2].type = SCREEN_EVENT_RECT;
    screen_q[2].payload.d.d1 = 1;
    screen_q[2].payload.d.d2 = 1;
    screen_q[2].payload.d.d3 = 62;
    screen_q[2].payload.d.d4 = 126;
    screen_q[3].type = SCREEN_EVENT_UPDATE;
    screen_q[4].type = SCREEN_EVENT_UPDATE;
    screen_q[5].type = SCREEN_EVENT_UPDATE;
    screen_q[6].type = SCREEN_EVENT_UPDATE;
    screen_q[7].type = SCREEN_EVENT_UPDATE;

    screen_q_wr = 8;
    screen_q_rd = 0;
}

// assumption: Q is locked
void screen_events_grow_queue() {
    if (screen_q_size >= SCREEN_Q_SIZE_MAX) {
        static struct timespec last_log = {0};
        struct timespec now = {0};

        clock_gettime(CLOCK_MONOTONIC, &now);

        // NOTE: If the Q is attempting to grow often and has already reached
        //       its limit, then it is likely that this log may flood the
        //       output. Put a time-based limiter on it for that case.

        if ((now.tv_sec - last_log.tv_sec) > 1) {
            fprintf(stderr, "SCREEN: error can't grow Q any more\n");
            last_log = now;
        }

        return;
    }

    const size_t new_screen_q_size = screen_q_size * 2;
    const size_t new_screen_q_mask = new_screen_q_size - 1;

    fprintf(stderr, "SCREEN: increasing Q size from %zu to %zu\n", screen_q_size, new_screen_q_size);

    struct screen_event_data * new_screen_q = realloc(screen_q, new_screen_q_size * sizeof(struct screen_event_data));

    if (new_screen_q == NULL) {
        fprintf(stderr, "SCREEN: grow error reallocating Q\n");
        return;
    }

    // NOTE: realloc() doesn't set new memory to 0s, so the assert in screen_event_data_move() will fail if we don't
    //       initialize the data first.

    for (size_t i = screen_q_size; i < new_screen_q_size; i++) {
        struct screen_event_data * ev = &new_screen_q[i];
        screen_event_data_init(ev);
    }

    screen_q_size = new_screen_q_size;
    screen_q_mask = new_screen_q_mask;
    screen_q = new_screen_q;
}

// assumption: Q is locked
void screen_events_shrink_queue() {
    if (screen_q_size <= SCREEN_Q_SIZE_INIT) {
        return;
    }

    const size_t new_screen_q_size = SCREEN_Q_SIZE_INIT;
    const size_t new_screen_q_mask = SCREEN_Q_MASK_INIT;

    fprintf(stderr, "SCREEN: decreasing Q size from %zu to %zu\n", screen_q_size, new_screen_q_size);

    // NOTE: We need to free event data that may still exist in the event queue before we lose
    //       their pointers.

    for (size_t i = new_screen_q_size; i < screen_q_size; i++) {
        struct screen_event_data * ev = &screen_q[i];
        screen_event_data_free(ev);
    }

    #ifndef MIN
    #define MIN(a,b) (((a)<(b))?(a):(b))
    screen_q_wr = MIN(screen_q_wr, new_screen_q_mask);
    screen_q_rd = MIN(screen_q_rd, new_screen_q_mask);
    #undef MIN
    #endif

    struct screen_event_data * new_screen_q = realloc(screen_q, new_screen_q_size * sizeof(struct screen_event_data));

    if (new_screen_q == NULL) {
        fprintf(stderr, "SCREEN: shrink error reallocating Q\n");
        return;
    }

    screen_q_size = new_screen_q_size;
    screen_q_mask = new_screen_q_mask;
    screen_q = new_screen_q;
}