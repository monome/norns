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
#include "battery.h"
#include "stat.h"
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

/// helpers
static void handle_engine_report(void);
//static void handle_command_report(void);
//static void handle_poll_report(void);

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
    union event_data *ev = calloc(1, sizeof(union event_data));
    ev->type = type;
    return ev;
}

void event_data_free(union event_data *ev) {
    switch (ev->type) {
    case EVENT_EXEC_CODE_LINE:
        free(ev->exec_code_line.line);
        break;
    case EVENT_OSC:
        free(ev->osc_event.path);
        free(ev->osc_event.from_host);
        free(ev->osc_event.from_port);
        lo_message_free(ev->osc_event.msg);
        break;
    case EVENT_POLL_DATA:
        free(ev->poll_data.data);
        break;
    case EVENT_POLL_WAVE:
        free(ev->poll_wave.data);
        break;
    case EVENT_SYSTEM_CMD:
        free(ev->system_cmd.capture);
        break;
    }
    free(ev);
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
        // fprintf(stderr, "evq.size : %d\n", (int) evq.size);
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
        w_handle_exec_code_line(ev->exec_code_line.line);
        break;
    case EVENT_METRO:
        w_handle_metro(ev->metro.id, ev->metro.stage);
        break;
    case EVENT_CLOCK_RESUME:
        w_handle_clock_resume(ev->clock_resume.thread_id);
        break;
    case EVENT_KEY:
        w_handle_key(ev->key.n, ev->key.val);
        break;
    case EVENT_ENC:
        w_handle_enc(ev->enc.n, ev->enc.delta);
        break;
    case EVENT_BATTERY:
        w_handle_battery(ev->battery.percent,
                         ev->battery.current);
        break;
    case EVENT_POWER:
        w_handle_power(ev->power.present);
        break;
    case EVENT_STAT:
        w_handle_stat(ev->stat.disk, ev->stat.temp, ev->stat.cpu);
        break;
    case EVENT_MONOME_ADD:
        w_handle_monome_add(ev->monome_add.dev);
        break;
    case EVENT_MONOME_REMOVE:
        w_handle_monome_remove(ev->monome_remove.id);
        break;
    case EVENT_GRID_KEY:
        w_handle_grid_key(ev->grid_key.id,
                          ev->grid_key.x,
                          ev->grid_key.y,
                          ev->grid_key.state);
        break;
    case EVENT_ARC_ENCODER_DELTA:
        w_handle_arc_encoder_delta(ev->arc_encoder_delta.id,
                                   ev->arc_encoder_delta.number,
                                   ev->arc_encoder_delta.delta);
        break;
    case EVENT_ARC_ENCODER_KEY:
        w_handle_arc_encoder_key(ev->arc_encoder_key.id,
                                 ev->arc_encoder_key.number,
                                 ev->arc_encoder_key.state);
        break;
    case EVENT_HID_ADD:
        w_handle_hid_add(ev->hid_add.dev);
        break;
    case EVENT_HID_REMOVE:
        w_handle_hid_remove(ev->hid_remove.id);
        break;
    case EVENT_HID_EVENT:
        w_handle_hid_event(ev->hid_event.id,
                           ev->hid_event.type,
                           ev->hid_event.code,
                           ev->hid_event.value);
        break;
    case EVENT_MIDI_ADD:
        w_handle_midi_add(ev->midi_add.dev);
        break;
    case EVENT_MIDI_REMOVE:
        w_handle_midi_remove(ev->midi_remove.id);
        break;
    case EVENT_MIDI_EVENT:
        w_handle_midi_event(ev->midi_event.id, ev->midi_event.data, ev->midi_event.nbytes);
        break;
    case EVENT_OSC:
        w_handle_osc_event(ev->osc_event.from_host,
            ev->osc_event.from_port,
            ev->osc_event.path,
            ev->osc_event.msg);
        break;
    case EVENT_ENGINE_REPORT:
        handle_engine_report();
        break;
    /* case EVENT_COMMAND_REPORT: */
    /*     handle_command_report(); */
    /*     break; */
    /* case EVENT_POLL_REPORT: */
    /*     handle_poll_report(); */
    /*     break; */
    case EVENT_ENGINE_LOADED:
      w_handle_engine_loaded();
      break;
    case EVENT_POLL_VALUE:
        w_handle_poll_value(ev->poll_value.idx, ev->poll_value.value);
        break;
    case EVENT_POLL_DATA:
        w_handle_poll_data(ev->poll_data.idx,
                           ev->poll_data.size,
                           ev->poll_data.data);
        break;
    case EVENT_POLL_IO_LEVELS:
        w_handle_poll_io_levels(ev->poll_io_levels.value.bytes);
        break;
    case EVENT_POLL_SOFTCUT_PHASE:
        w_handle_poll_softcut_phase(ev->softcut_phase.idx, ev->softcut_phase.value);
        break;
    case EVENT_STARTUP_READY_OK:
        w_handle_startup_ready_ok();
        break;
    case EVENT_STARTUP_READY_TIMEOUT:
        w_handle_startup_ready_timeout();
        break;
    case EVENT_SYSTEM_CMD:
        w_handle_system_cmd(ev->system_cmd.capture);
        break;
    case EVENT_RESET_LVM:
        w_reset_lvm();
        break;
    case EVENT_QUIT:
        quit = true;
        break;
    case EVENT_CROW_ADD:
        w_handle_crow_add(ev->crow_add.dev);
        break;
    case EVENT_CROW_REMOVE:
        w_handle_crow_remove(ev->crow_remove.id);
        break;
    case EVENT_CROW_EVENT:
        w_handle_crow_event(ev->crow_event.dev, ev->crow_event.id);
        break;
    } /* switch */

    event_data_free(ev);
}

//---------------------------------
//---- helpers

//--- reports
void handle_engine_report(void) {
    o_lock_descriptors();
    const char **p = o_get_engine_names();
    const int n = o_get_num_engines();
    w_handle_engine_report(p, n);
    o_unlock_descriptors();
}

