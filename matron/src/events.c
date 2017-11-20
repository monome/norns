#include <assert.h>
#include <search.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "events.h"
#include "device_monome.h"
#include "gpio.h"
#include "oracle.h"
#include "weaver.h"

#include "event_types.h"

//----------------------------
//--- types and variables

struct ev_node {
    struct ev_node *next;
    struct ev_node *prev;
    union event_data *ev;
};

struct ev_q {
    struct ev_node *head;
    struct ev_node *tail;
    ssize_t size;
    pthread_cond_t nonempty;
    pthread_mutex_t lock;
};

struct ev_q evq;
bool quit;

//----------------------------
//--- static function declarations

//---- handlers
static void handle_event(union event_data *ev);
static void handle_exec_code_line(struct event_exec_code_line *ev);
static void handle_timer(struct event_timer *ev);
static void handle_gpio(struct event_gpio *ev);
static void handle_monome_add(struct event_monome_add *ev);
static void handle_monome_remove(struct event_monome_remove *ev);
static void handle_grid_key(struct event_grid_key *ev);
static void handle_hid_add(struct event_hid_add *ev);
static void handle_hid_remove(struct event_hid_remove *ev);
static void handle_hid_event(struct event_hid_event *ev);
static void handle_engine_report(void);
static void handle_command_report(void);
static void handle_poll_report(void);
static void handle_poll_value(struct event_poll_value *ev);
static void handle_poll_data(struct event_poll_data *ev);
static void handle_quit(void);

// add an event data struct to the end of the event queue
// *does* allocate queue node memory!
// *does not* allocate event data memory!
// call with the queue locked
static void evq_push(union event_data *ev) {
    struct ev_node *evn = calloc( 1, sizeof(struct ev_node) );
    evn->ev = ev;
    if(evq.size == 0) {
        insque(evn, NULL);
        evq.head = evn;
    } else {
        insque(evn, evq.tail);
    }
    evq.tail = evn;
    evq.size += 1;
}

// remove and return the event data struct from the top of the event queue
// call with the queue locked
// *does* free queue node memory!
// *does not* free the event data memory!
static union event_data *evq_pop() {
    struct ev_node *evn = evq.head;
    if(evn == NULL) {
        return NULL;
    }
    union event_data *ev = evn->ev;
    evq.head = evn->next;
    if(evn == evq.tail) {
        assert(evq.size == 1);
        evq.tail = NULL;
    }
    remque(evn);
    free(evn);
    evq.size -= 1;
    return ev;
}

//-------------------------------
//-- extern function definitions

void events_init(void) {
    evq.size = 0;
    evq.head = NULL;
    evq.tail = NULL;
    pthread_cond_init(&evq.nonempty, NULL);
}

union event_data *event_data_new(event_t type) {
    // FIXME: better not to allocate here, use object pool
    union event_data *ev = calloc( 1, sizeof(union event_data) );
    ev->type = type;
    return ev;
}

// add an event to the q and signal if necessary
void event_post(union event_data *ev) {
    assert(ev != NULL);
    pthread_mutex_lock(&evq.lock);
    if(evq.size == 0) {
        // signal handler thread to wake up...
        pthread_cond_signal(&evq.nonempty);
    }
    evq_push(ev);
    // ...handler actually wakes up once we release the lock
    pthread_mutex_unlock(&evq.lock);
}

// main loop to read events!
void event_loop(void) {
    union event_data *ev;
    while(!quit) {
        pthread_mutex_lock(&evq.lock);
        // while() because contention may produce spurious wakeup
        while(evq.size == 0) {
            //// FIXME: if we have an input device thread running,
            //// then we get segfaults here on SIGINT
            //// need to set an explicit sigint handler
            // atomically unlocks the mutex, sleeps on condvar, locks again on
            // wakeup
            pthread_cond_wait(&evq.nonempty, &evq.lock);
        }
        // printf("evq.size : %d\n", (int) evq.size); fflush(stdout);
        assert(evq.size > 0);
        ev = evq_pop();
        pthread_mutex_unlock(&evq.lock);
        if(ev != NULL) {
            handle_event(ev);
        }
    }
}

//------------------------------
//-- static function definitions

static void handle_event(union event_data *ev) {
    switch(ev->type) {
    case EVENT_EXEC_CODE_LINE:
        handle_exec_code_line( &(ev->exec_code_line) );
        break;
    case EVENT_TIMER:
        handle_timer( &(ev->timer) );
        break;
    case EVENT_GPIO:
        handle_gpio( &(ev->gpio) );
        break;
    case EVENT_MONOME_ADD:
        handle_monome_add( &(ev->monome_add) );
        break;
    case EVENT_MONOME_REMOVE:
        handle_monome_remove( &(ev->monome_remove) );
        break;
    case EVENT_GRID_KEY:
        handle_grid_key( &(ev->grid_key) );
        break;
    case EVENT_HID_ADD:
        handle_hid_add( &(ev->hid_add) );
        break;
    case EVENT_HID_REMOVE:
        handle_hid_remove( &(ev->hid_remove) );
        break;
    case EVENT_HID_EVENT:
        handle_hid_event( &(ev->hid_event) );
        break;
    case EVENT_ENGINE_REPORT:
        handle_engine_report();
        break;
    case EVENT_COMMAND_REPORT:
        handle_command_report();
        break;
    case EVENT_POLL_REPORT:
        handle_poll_report();
        break;
    case EVENT_POLL_VALUE:
        handle_poll_value( &(ev->poll_value) );
        break;
    case EVENT_POLL_DATA:
        handle_poll_data( &(ev->poll_data) );
        break;
    case EVENT_QUIT:
        handle_quit();
        break;
    } /* switch */
}

//---------------------------------
//---- handlers

//--- code execution
void handle_exec_code_line(struct event_exec_code_line *ev) {
    w_handle_line(ev->line);
    free(ev->line);
}

//--- timers
void handle_timer(struct event_timer *ev) {
    w_handle_timer(ev->id, ev->stage);
}

//--- gpio
void handle_gpio(struct event_gpio *ev) {
    w_handle_gpio(ev->pin, ev->val);
}

//--- monome devices
void handle_monome_add(struct event_monome_add *ev) {
    w_handle_monome_add(ev->dev);
}

void handle_monome_remove(struct event_monome_remove *ev) {
    w_handle_monome_remove(ev->id);
}

void handle_grid_key(struct event_grid_key *ev) {
    w_handle_grid_key(ev->id, ev->x, ev->y, ev->state);
    fflush(stdout);
}

//--- hid devices
void handle_hid_add(struct event_hid_add *ev) {
    w_handle_hid_add(ev->dev);
}

void handle_hid_remove(struct event_hid_remove *ev) {
    w_handle_hid_remove(ev->id);
}

void handle_hid_event(struct event_hid_event *ev) {
    w_handle_hid_event(ev->id, ev->type, ev->code, ev->value);
}

//--- TODO: MIDI

//--- reports
void handle_engine_report(void) {
    o_lock_descriptors();
    const char **p = o_get_engine_names();
    const int n = o_get_num_engines();
    w_handle_engine_report(p, n);
    o_unlock_descriptors();
}

void handle_command_report(void) {
    o_lock_descriptors();
    const struct engine_command *p = o_get_commands();
    const int n = o_get_num_commands();
    printf("handling command report with %d commands \n", n);
    w_handle_command_report(p, n);
    o_unlock_descriptors();
}

void handle_poll_report(void) {
    o_lock_descriptors();
    const struct engine_poll *p = o_get_polls();
    const int n = o_get_num_polls();
    w_handle_poll_report(p, n);
    o_unlock_descriptors();
};

void handle_poll_value(struct event_poll_value *ev) {
    // printf("(events) handle_poll_value; idx: %d; value: %f\n", ev->idx,
    // ev->value); fflush(stdout);
    w_handle_poll_value(ev->idx, ev->value);
}

void handle_poll_data(struct event_poll_data *ev) {
    w_handle_poll_data(ev->idx, ev->size, ev->data);
    free(ev->data);
}

//--- quit
void handle_quit(void) {
    quit = true;
}
