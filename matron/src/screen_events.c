#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "screen_events.h"
#include "screen_events_pr.h"
#include "screen.h"

#define DEBUG_SCREEN_EVENTS 0

#define SCREEN_Q_SIZE 1024
#define SCREEN_Q_MASK (SCREEN_Q_SIZE -1)

static struct screen_event_data screen_q[SCREEN_Q_SIZE];
static int screen_q_wr = 0;
static int screen_q_rd = 0;

static void* screen_event_loop(void*);

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
// call with the Q locked
static void screen_events_emergency_clear();

void screen_events_init() { 
    pthread_cond_init(&screen_q_nonempty, NULL);
    if (pthread_create(&screen_event_thread, NULL, screen_event_loop, 0) ) {
	fprintf(stderr, "SCREEN: error creating thread\n");
    }
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

static void screen_event_data_move(struct screen_event_data *dst,
				   struct screen_event_data *src) {
    assert(dst->buf == NULL && dst->type == SCREEN_EVENT_NONE);
    *dst = *src;
    screen_event_data_init(src);
}

// call from any thread; locks Q (blocking)
void screen_event_data_push(struct screen_event_data *src) {
#if DEBUG_SCREEN_EVENTS
    fprintf(stderr, "screen_event_q push; wr=%d; rd=%d\n", screen_q_wr, screen_q_rd);
#endif
    pthread_mutex_lock(&screen_q_lock);
    struct screen_event_data* dst = &(screen_q[screen_q_wr]);
    screen_event_data_move(dst, src);
    screen_q_wr = (screen_q_wr + 1) & SCREEN_Q_MASK;
    // if these indices become equal here, we've filled the queue
    if(screen_q_wr == screen_q_rd) {
        // TODO: post some kind of error to lua?
	fprintf(stderr, "warning: screen event Q full! \n");
	screen_events_emergency_clear();
    }
    pthread_cond_signal(&screen_q_nonempty);
    pthread_mutex_unlock(&screen_q_lock);
}

// call with Q locked from screen handler thread
void screen_event_data_pop(struct screen_event_data *dst) {
#if DEBUG_SCREEN_EVENTS
    fprintf(stderr, "screen_event_q pop; rd=%d; wr=%d\n", screen_q_rd, screen_q_wr);
#endif
    struct screen_event_data *src = &screen_q[screen_q_rd];
    screen_event_data_move(dst, src);
    screen_q_rd = (screen_q_rd + 1) & SCREEN_Q_MASK;
}

void* screen_event_loop(void* x) { 
    (void)x;
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    while(1) {
        pthread_mutex_lock(&screen_q_lock);
        while (screen_q_rd == screen_q_wr) {
            pthread_cond_wait(&screen_q_nonempty, &screen_q_lock);
        }
        assert (screen_q_rd != screen_q_wr);
        screen_event_data_pop(&ev);
        pthread_mutex_unlock(&screen_q_lock);
        handle_screen_event(&ev);
    }
}

#if DEBUG_SCREEN_EVENTS
static void print_event_type(struct screen_event_data *ev) {
    switch(ev->type) {
    case SCREEN_EVENT_UPDATE:
	fprintf(stderr, "SCREEN_EVENT_UPDATE\n");
	break;
    case SCREEN_EVENT_SAVE:
	fprintf(stderr, "SCREEN_EVENT_SAVE\n");
	break;
    case SCREEN_EVENT_RESTORE:
	fprintf(stderr, "SCREEN_EVENT_RESTORE\n");
	break;
    case SCREEN_EVENT_FONT_FACE:
	fprintf(stderr, "SCREEN_EVENT_FONT_FACE\n");
	break;
    case SCREEN_EVENT_FONT_SIZE:
	fprintf(stderr, "SCREEN_EVENT_FONT_SIZE\n");
	break;
    case SCREEN_EVENT_AA:
	fprintf(stderr, "SCREEN_EVENT_AA\n");
	break;
    case SCREEN_EVENT_LEVEL:
	fprintf(stderr, "SCREEN_EVENT_LEVEL\n");
	break;
    case SCREEN_EVENT_LINE_WIDTH:
	fprintf(stderr, "SCREEN_EVENT_LINE_WIDTH\n");
	break;
    case SCREEN_EVENT_LINE_CAP:
	fprintf(stderr, "SCREEN_EVENT_LINE_CAP\n");
	break;
    case SCREEN_EVENT_LINE_JOIN:
	fprintf(stderr, "SCREEN_EVENT_LINE_JOIN\n");
	break;
    case SCREEN_EVENT_MITER_LIMIT:
	fprintf(stderr, "SCREEN_EVENT_MITER_LIMIT\n");
	break;
    case SCREEN_EVENT_MOVE:
	fprintf(stderr, "SCREEN_EVENT_MOVE\n");
	break;
    case SCREEN_EVENT_LINE:
	fprintf(stderr, "SCREEN_EVENT_LINE\n");
	break;
    case SCREEN_EVENT_MOVE_REL:
	fprintf(stderr, "SCREEN_EVENT_MOVE_REL\n");
	break;
    case SCREEN_EVENT_LINE_REL:
	fprintf(stderr, "SCREEN_EVENT_LINE_REL\n");
	break;
    case SCREEN_EVENT_CURVE:
	fprintf(stderr, "SCREEN_EVENT_CURVE\n");
	break;
    case SCREEN_EVENT_CURVE_REL:
	fprintf(stderr, "SCREEN_EVENT_CURVE_REL\n");
	break;
    case SCREEN_EVENT_ARC:
	fprintf(stderr, "SCREEN_EVENT_ARC\n");
	break;
    case SCREEN_EVENT_RECT:
	fprintf(stderr, "SCREEN_EVENT_RECT\n");
	break;
    case SCREEN_EVENT_STROKE:
	fprintf(stderr, "SCREEN_EVENT_STROKE\n");
	break;
    case SCREEN_EVENT_FILL:
	fprintf(stderr, "SCREEN_EVENT_FILL\n");
	break;
    case SCREEN_EVENT_TEXT:
	fprintf(stderr, "SCREEN_EVENT_TEXT\n");
	break;
    case SCREEN_EVENT_TEXT_EXTENTS:
	fprintf(stderr, "SCREEN_EVENT_TEXT_EXTENTS\n");
	break;
    case SCREEN_EVENT_CLEAR:
	fprintf(stderr, "SCREEN_EVENT_CLEAR\n");
	break;
    case SCREEN_EVENT_CLOSE_PATH:
	fprintf(stderr, "SCREEN_EVENT_CLOSE_PATH\n");
	break;
    case SCREEN_EVENT_EXPORT_PNG:
	fprintf(stderr, "SCREEN_EVENT_EXPORT_PNG\n");
	break;
    case SCREEN_EVENT_DISPLAY_PNG:
	fprintf(stderr, "SCREEN_EVENT_DISPLAY_PNG\n");
	break;
    case SCREEN_EVENT_ROTATE:
	fprintf(stderr, "SCREEN_EVENT_ROTATE\n");
	break;
    case SCREEN_EVENT_TRANSLATE:
	fprintf(stderr, "SCREEN_EVENT_TRANSLATE\n");
	break;
    case SCREEN_EVENT_SET_OPERATOR:
	fprintf(stderr, "SCREEN_EVENT_SET_OPERATOR\n");
	break;
    case SCREEN_EVENT_PEEK:
	fprintf(stderr, "SCREEN_EVENT_PEEK\n");
	break;
    case SCREEN_EVENT_POKE:
	fprintf(stderr, "SCREEN_EVENT_POKE\n");
	break;
    case SCREEN_EVENT_CURRENT_POINT:
	fprintf(stderr, "SCREEN_EVENT_CURRENT_POINT\n");
	break;
    }
}
#endif

void handle_screen_event(struct screen_event_data *ev) {
#if DEBUG_SCREEN_EVENTS
    print_event_type(ev);
#endif
    assert(ev->type != SCREEN_EVENT_NONE);
    switch(ev->type) {
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
	screen_curve(ev->payload.d.d1, ev->payload.d.d2,
		     ev->payload.d.d3, ev->payload.d.d4,
		     ev->payload.d.d5, ev->payload.d.d6);
	break;
    case SCREEN_EVENT_CURVE_REL:
	screen_curve_rel(ev->payload.d.d1, ev->payload.d.d2,
			 ev->payload.d.d3, ev->payload.d.d4,
			 ev->payload.d.d5, ev->payload.d.d6);
	break;
    case SCREEN_EVENT_ARC:
	screen_arc(ev->payload.d.d1, ev->payload.d.d2,
		   ev->payload.d.d3, ev->payload.d.d4,
		   ev->payload.d.d5);
	break;
    case SCREEN_EVENT_RECT:
	screen_rect(ev->payload.d.d1, ev->payload.d.d2,
		    ev->payload.d.d3, ev->payload.d.d4);
	break;
    case SCREEN_EVENT_STROKE:
	screen_stroke();
	break;
    case SCREEN_EVENT_FILL:
	screen_fill();
	break;
    case SCREEN_EVENT_TEXT:
#if DEBUG_SCREEN_EVENTS
	assert(ev->payload.b.nb == strlen(ev->buf)+1);
#endif
	screen_text(ev->buf);
	break;
    case SCREEN_EVENT_TEXT_EXTENTS:
	screen_text_extents(ev->buf);
	break;
    case SCREEN_EVENT_CLEAR:
	screen_clear();
	break;
    case SCREEN_EVENT_CLOSE_PATH:
	screen_close_path();
	break;
    case SCREEN_EVENT_EXPORT_PNG:
#if DEBUG_SCREEN_EVENTS
	assert(ev->payload.b.nb == strlen(ev->buf)+1);
#endif
	screen_export_png(ev->buf);
	break;
    case SCREEN_EVENT_DISPLAY_PNG:
#if DEBUG_SCREEN_EVENTS
	assert(ev->payload.bd.nb == strlen(ev->buf)+1);
#endif
	screen_display_png(ev->buf, ev->payload.bd.d1, ev->payload.bd.d2);
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
#if DEBUG_SCREEN_EVENTS
	assert(ev->payload.bi.nb == ev->payload.bi.i3 * ev->payload.bi.i4);
#endif
	screen_poke(ev->payload.bi.i1, ev->payload.bi.i2,
		    ev->payload.bi.i3, ev->payload.bi.i4,
		    ev->buf);
	break;
    case SCREEN_EVENT_CURRENT_POINT:
	screen_current_point();
	break;
    default:
	;;
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
#if DEBUG_SCREEN_EVENTS
    fprintf(stderr, "screen_event_copy_string: string = \"%s\"; nb = %d\n", s, nb);
#endif
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

void screen_event_text_extents(const char *s) {
    struct screen_event_data ev;
    screen_event_data_init(&ev);
    ev.type = SCREEN_EVENT_TEXT_EXTENTS;
    screen_event_copy_string(&ev, s);
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
    size_t nb  = w * h;
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

// assumption: Q is locked
void screen_events_emergency_clear() {
    struct screen_event_data *ev;
    for (int i=0; i<SCREEN_Q_SIZE; ++i) {
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
