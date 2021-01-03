
#include <pthread.h>

#include "screen_events.h"

#define SCREEN_EVENT_Q_SIZE 128
#define SCREEN_EVENT_Q_MASK (SCREEN_EVENT_Q_SIZE -1)

static screen_event_data_t screen_event_q[SCREEN_EVENT_Q_SIZE];
static int screen_event_q_idx = 0;
static int screen_event_q_count = 0;

static void handle_screen_event(screen_event_data_t *dst, screen_event_data_t *src);

void screen_event_push(screen_event_id_t type, int arg_count, ...) {
    va_list args;
    va_start(args, arg_count);

    // TODO: lock the queue
    screen_event_data_t* data = &(screen_event_q[screen_event_q_idx]); 
    data->type = type;
    data->data_count = arg_count;
    for (int i=0; i<arg_count; ++i) { 
        data->data[i] = va_arg(args, double);
    }
    va_end(args);

    screen_event_q_idx = (screen_event_q_idx + 1) & SCREEN_EVENT_Q_MASK;
    screen_event_q_count++;

    // TODO: unlock the queue
    // TODO: wakeup the consumer thread
}

// call with q locked
__attribute__((unused)) 
void handle_screen_event(screen_event_data_t *dst, screen_event_data_t *src) {
    dst->type = src->type;
    dst->data_count = src->data_count;
    for (int i=0; i<src->data_count; ++i) {
        dst->data[i] = src->data[i];
    }
    screen_event_q_count--;
}
