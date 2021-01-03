#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "screen_events.h"
#include "screen.h"

#define SCREEN_Q_SIZE 128
#define SCREEN_Q_MASK (SCREEN_Q_SIZE -1)

static screen_event_data_t screen_q[SCREEN_Q_SIZE];
static int screen_q_wr = 0;
static int screen_q_rd = 0;

static void screen_event_pop(screen_event_data_t *dst);
static void handle_screen_event(const screen_event_data_t *data);

static pthread_mutex_t screen_q_lock;
static pthread_cond_t screen_q_nonempty;

void screen_events_init() { 
    pthread_cond_init(&screen_q_nonempty, NULL);
}

void screen_event_push(screen_event_id_t type, const char *text, int data_count, ...) {
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

    pthread_cond_signal(&screen_q_nonempty);
    pthread_mutex_unlock(&screen_q_lock);
}

// call with q locked
__attribute__((unused)) 
void screen_event_pop(screen_event_data_t *dst) {
    //------------------------------------------------
    // FIXME?: this bit doesn't really need to be in the lock
    if (dst->text != NULL) {
        free(dst->text);
        dst->text = NULL;
    }
    //-----------------------------------

    screen_event_data_t *src = &screen_q[screen_q_rd];
    dst->type = src->type;
    dst->data_count = src->data_count;
    for (int i=0; i<src->data_count; ++i) {
        dst->data[i] = src->data[i];
    }
    if (src->text != NULL) {
        int nbytes = strlen(src->text) + 1;
        dst->text = malloc(nbytes);
        memcpy(dst->text, src->text, nbytes);
        free(src->text);
    }
    screen_q_rd = (screen_q_rd + 1) & SCREEN_Q_MASK;
}

__attribute__((unused)) 
void screen_event_loop() { 
    screen_event_data_t event_data;
    event_data.text = NULL;
    event_data.data_count = 0;
    while(1) {
        pthread_mutex_lock(&screen_q_lock);
        while (screen_q_rd == screen_q_wr) { 
            pthread_cond_wait(&screen_q_nonempty, &screen_q_lock);
        }
        assert (screen_q_rd != screen_q_wr);
        screen_event_pop(&event_data);
        pthread_mutex_unlock(&screen_q_lock);
        handle_screen_event(&event_data);
    }
}


void handle_screen_event(const screen_event_data_t *event) {
    switch(event->type) {
        case SCREEN_EVENT_SAVE:
        break;
        case SCREEN_EVENT_RESTORE:
        break;
        case SCREEN_EVENT_FONT_FACE:
        break;
        case SCREEN_EVENT_FONT_SIZE:
        break;
        case SCREEN_EVENT_AA:
        break;
        case SCREEN_EVENT_LEVEL:
        break;
        case SCREEN_EVENT_LINE_WIDTH:
        break;
        case SCREEN_EVENT_LINE_CAP:
        break;
        case SCREEN_EVENT_LINE_JOIN:
        break;
        case SCREEN_EVENT_MITER_LIMIT:
        break;
        case SCREEN_EVENT_MOVE:
        break;
        case SCREEN_EVENT_LINE:
        break;
        case SCREEN_EVENT_MOVE_REL:
        break;
        case SCREEN_EVENT_LINE_REL:
        break;
        case SCREEN_EVENT_CURVE:
        break;
        case SCREEN_EVENT_CURVE_REL:
        break;
        case SCREEN_EVENT_ARC:
        break;
        case SCREEN_EVENT_RECT:
        break;
        case SCREEN_EVENT_STROKE:
        break;
        case SCREEN_EVENT_FILL:
        break;
        case SCREEN_EVENT_TEXT:
        break;
        case SCREEN_EVENT_CLEAR:
        break;
        case SCREEN_EVENT_CLOSE_PATH:
        break;
        case SCREEN_EVENT_EXTENTS:
        break;
        case SCREEN_EVENT_EXPORT_PNG:
        break;
        case SCREEN_EVENT_DISPLAY_PNG:
        break;
        default:
            ;;
    }
}