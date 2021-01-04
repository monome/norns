#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "screen_events.h"
#include "screen.h"

#define SCREEN_Q_SIZE 1024
#define SCREEN_Q_MASK (SCREEN_Q_SIZE -1)

static screen_event_data_t screen_q[SCREEN_Q_SIZE];
static int screen_q_wr = 0;
static int screen_q_rd = 0;

static void* screen_event_loop(void*);
static void screen_event_pop(screen_event_data_t *dst);
static void screen_event_free(screen_event_data_t *dst);
static void handle_screen_event(const screen_event_data_t *data);

static pthread_t screen_event_thread;
static pthread_mutex_t screen_q_lock;
static pthread_cond_t screen_q_nonempty;

void screen_events_init() { 
    pthread_cond_init(&screen_q_nonempty, NULL);
    // set up screen event loop
    if (pthread_create(&screen_event_thread, NULL, screen_event_loop, 0) ) {
      fprintf(stderr, "SCREEN: error creating thread\n");
    }
}

void screen_event_push(screen_event_id_t type, const char *text, int data_count, ...) {
    assert(data_count <= SCREEN_EVENT_DATA_COUNT);
    va_list args;
    pthread_mutex_lock(&screen_q_lock);
    screen_event_data_t* data = &(screen_q[screen_q_wr]); 
    data->type = type;
    data->data_count = data_count;
    if (text != NULL) {
        int nbytes = strlen(text) + 1;
        data->text = malloc(nbytes);
        memcpy(data->text, text, nbytes);
    } else {
        data->text = NULL;
    }
    va_start(args, data_count);
    for (int i=0; i<data_count; ++i) { 
        data->data[i] = va_arg(args, double);
    }
    va_end(args);
    screen_q_wr = (screen_q_wr + 1) & SCREEN_Q_MASK;
    // if these indices become equal, we've filled the queue
    if(screen_q_wr == screen_q_rd) {
        fprintf(stderr, "ERROR: screen event queue is full!\n");

#if 1 // should we continue, overwriting the oldest message?
        screen_event_free(&screen_q[screen_q_wr]);
        screen_q_rd = (screen_q_rd + 1) & SCREEN_Q_MASK;
#else // .. or, just crash?
        assert(0);
#endif
    }
    pthread_cond_signal(&screen_q_nonempty);
    pthread_mutex_unlock(&screen_q_lock);
}

// call with q locked
void screen_event_pop(screen_event_data_t *dst) {
    screen_event_data_t *src = &screen_q[screen_q_rd];
    dst->type = src->type;
    dst->data_count = src->data_count;
    for (int i=0; i<src->data_count; ++i) {
        dst->data[i] = src->data[i];
    }
    if (src->text != NULL) {
        dst->text = src->text;
        src->text = NULL;
    }
    screen_q_rd = (screen_q_rd + 1) & SCREEN_Q_MASK;
}

void* screen_event_loop(void* x) { 
    (void)x;
    screen_event_data_t event_data;
    event_data.text = NULL;
    event_data.data_count = 0;
    for (int i=0; i<6; ++i) { event_data.data[i] = 0.0; }
    while(1) {
        pthread_mutex_lock(&screen_q_lock);
        while (screen_q_rd == screen_q_wr) { 
            pthread_cond_wait(&screen_q_nonempty, &screen_q_lock);
        }
        assert (screen_q_rd != screen_q_wr);
        screen_event_pop(&event_data);
        pthread_mutex_unlock(&screen_q_lock);
        handle_screen_event(&event_data);
        screen_event_free(&event_data);
    }
}

void screen_event_free(screen_event_data_t *ev) {
    if (ev->text != NULL) { 
        free(ev->text);
        ev->text = NULL;
    }
    ev->data_count = 0;
}

void handle_screen_event(const screen_event_data_t *event) {
    switch(event->type) {
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
            screen_font_face((int)event->data[0]);
            break;
        case SCREEN_EVENT_FONT_SIZE:
            screen_font_size(event->data[0]);
            break;
        case SCREEN_EVENT_AA:
	        screen_aa((int)event->data[0]);
            break;
        case SCREEN_EVENT_LEVEL:
	        screen_level((int)event->data[0]);
            break;
        case SCREEN_EVENT_LINE_WIDTH:
	        screen_line_width(event->data[0]);
            break;
        case SCREEN_EVENT_LINE_CAP:
	        screen_line_cap(event->text);
            break;
        case SCREEN_EVENT_LINE_JOIN:
	        screen_line_join(event->text);
            break;
        case SCREEN_EVENT_MITER_LIMIT:
	        screen_miter_limit(event->data[0]);
            break;
        case SCREEN_EVENT_MOVE:
	        screen_move(event->data[0], event->data[1]);
            break;
        case SCREEN_EVENT_LINE:
	        screen_line(event->data[0], event->data[1]);
            break;
        case SCREEN_EVENT_MOVE_REL:
	        screen_move_rel(event->data[0], event->data[1]);
            break;
        case SCREEN_EVENT_LINE_REL:
	        screen_line_rel(event->data[0], event->data[1]);
            break;
        case SCREEN_EVENT_CURVE:
	        screen_curve(event->data[0], event->data[1], event->data[2], event->data[3], event->data[4], event->data[5]);
            break;
        case SCREEN_EVENT_CURVE_REL:
	        screen_curve_rel(event->data[0], event->data[1], event->data[2], event->data[3], event->data[4], event->data[5]);
            break;
        case SCREEN_EVENT_ARC:
	        screen_arc(event->data[0], event->data[1], event->data[2], event->data[3], event->data[4]);
            break;
        case SCREEN_EVENT_RECT:
	        screen_rect(event->data[0], event->data[1], event->data[2], event->data[3]);
            break;
        case SCREEN_EVENT_STROKE:
	        screen_stroke();
            break;
        case SCREEN_EVENT_FILL:
	        screen_fill();
            break;
        case SCREEN_EVENT_TEXT:
	        screen_text(event->text);
            break;
        case SCREEN_EVENT_CLEAR:
	        screen_clear();
            break;
        case SCREEN_EVENT_CLOSE_PATH:
	        screen_close_path();
            break;
        case SCREEN_EVENT_EXPORT_PNG:
	        screen_export_png(event->text);
            break;
        case SCREEN_EVENT_DISPLAY_PNG:
	        screen_display_png(event->text, event->data[0], event->data[1]);
            break;
        case SCREEN_ROTATE:
            screen_rotate(event->data[0]);
            break;
        case SCREEN_TRANSLATE:
            screen_translate(event->data[0], event->data[1]);
            break;
        case SCREEN_SET_OPERATOR:
            screen_set_operator((int)event->data[0]);
            break;
        default:
            ;;
    }

}