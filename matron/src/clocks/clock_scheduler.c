#include <float.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "../clock.h"
#include "../events.h"

#include "clock_scheduler.h"

typedef enum {
    CLOCK_SCHEDULER_EVENT_SYNC,
    CLOCK_SCHEDULER_EVENT_SLEEP,
} clock_scheduler_event_type_t;

typedef struct {
    clock_scheduler_event_type_t type;
    bool ready;
    int thread_id;

    double sync_beat;
    double sync_beat_offset;
    double sync_clock_beat;

    double sleep_time;
    double sleep_clock_time;
} clock_scheduler_event_t;

static pthread_t clock_scheduler_tick_thread;

static clock_scheduler_event_t clock_scheduler_events[NUM_CLOCK_SCHEDULER_EVENTS];
static pthread_mutex_t clock_scheduler_events_lock;

static void clock_scheduler_post_clock_resume_event(int thread_id, double value) {
    union event_data *ev = event_data_new(EVENT_CLOCK_RESUME);
    ev->clock_resume.thread_id = thread_id;
    ev->clock_resume.value = value;
    event_post(ev);
}

static double clock_scheduler_next_clock_beat(double clock_beat, double sync_beat, double sync_beat_offset) {
    double next_beat;

    next_beat = ceil((clock_beat + FLT_EPSILON) / sync_beat) * sync_beat;
    next_beat = next_beat + sync_beat_offset;

    while (next_beat < (clock_beat + FLT_EPSILON)) {
        next_beat += sync_beat;
    }

    return fmax(next_beat, 0);
}

static clock_scheduler_event_t *clock_scheduler_find_event(int thread_id) {
    int result = -1;

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        if (clock_scheduler_events[i].thread_id == thread_id) {
            result = i;
            break;
        }

        // if an event for the given thread_id was not found,
        // return the index for the first available event
        if (clock_scheduler_events[i].thread_id == -1 && result < 0) {
            result = i;
        }
    }

    if (result > -1) {
        return &clock_scheduler_events[result];
    }

    return NULL;
}

static void *clock_scheduler_tick_thread_run(void *p) {
    (void)p;
    clock_scheduler_event_t *event;
    double clock_beat;
    double clock_time;

    while (true) {
        pthread_mutex_lock(&clock_scheduler_events_lock);

        clock_time = clock_get_system_time();
        clock_beat = clock_get_beats();

        for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
            event = &clock_scheduler_events[i];

            if (event->ready) {
                if (event->type == CLOCK_SCHEDULER_EVENT_SYNC) {
                    if (clock_beat > event->sync_clock_beat) {
                        clock_scheduler_post_clock_resume_event(event->thread_id, clock_beat);
                        event->ready = false;
                    }
                } else {
                    if (clock_time >= event->sleep_clock_time) {
                        clock_scheduler_post_clock_resume_event(event->thread_id, clock_time);
                        event->ready = false;
                    }
                }
            };
        }

        pthread_mutex_unlock(&clock_scheduler_events_lock);
        usleep(1000);
    }

    return NULL;
}

void clock_scheduler_init() {
    pthread_mutex_init(&clock_scheduler_events_lock, NULL);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        clock_scheduler_events[i].ready = false;
        clock_scheduler_events[i].thread_id = -1;
    }

    clock_scheduler_start();
}

void clock_scheduler_start() {
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_create(&clock_scheduler_tick_thread, &attr, &clock_scheduler_tick_thread_run, NULL);
    pthread_attr_destroy(&attr);
}

bool clock_scheduler_schedule_sync(int thread_id, double sync_beat, double sync_beat_offset) {
    pthread_mutex_lock(&clock_scheduler_events_lock);

    double clock_beat = clock_get_beats();
    clock_scheduler_event_t *event = clock_scheduler_find_event(thread_id);

    if (event != NULL) {
        if (event->thread_id == thread_id) {
            event->ready = true;
            event->sync_beat = sync_beat;
            event->sync_beat_offset = sync_beat_offset;

            // count from the stored sync_clock_beat when syncing the same coroutine
            if (event->type == CLOCK_SCHEDULER_EVENT_SYNC) {
                event->sync_clock_beat = clock_scheduler_next_clock_beat(event->sync_clock_beat, sync_beat, sync_beat_offset);
            } else {
                event->sync_clock_beat = clock_scheduler_next_clock_beat(clock_beat, sync_beat, sync_beat_offset);
                event->type = CLOCK_SCHEDULER_EVENT_SYNC;
            }
        } else {
            event->thread_id = thread_id;
            event->ready = true;
            event->sync_beat = sync_beat;
            event->sync_clock_beat = clock_scheduler_next_clock_beat(clock_beat, sync_beat, sync_beat_offset);
            event->type = CLOCK_SCHEDULER_EVENT_SYNC;
        }

        pthread_mutex_unlock(&clock_scheduler_events_lock);
        return true;
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
    return false;
}

bool clock_scheduler_schedule_sleep(int thread_id, double seconds) {
    pthread_mutex_lock(&clock_scheduler_events_lock);

    double clock_time = clock_get_system_time();
    clock_scheduler_event_t *event = clock_scheduler_find_event(thread_id);

    if (event != NULL) {
        event->ready = true;
        event->thread_id = thread_id;
        event->sleep_time = seconds;
        event->sleep_clock_time = clock_time + seconds;
        event->type = CLOCK_SCHEDULER_EVENT_SLEEP;

        pthread_mutex_unlock(&clock_scheduler_events_lock);
        return true;
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
    return false;
}

void clock_scheduler_clear(int thread_id) {
    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        if (clock_scheduler_events[i].thread_id == thread_id) {
            clock_scheduler_events[i].ready = false;
            clock_scheduler_events[i].thread_id = -1;
        }
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
}

void clock_scheduler_clear_all() {
    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        clock_scheduler_events[i].ready = false;
        clock_scheduler_events[i].thread_id = -1;
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
}

void clock_scheduler_reschedule_sync_events() {
    clock_scheduler_event_t *event;

    pthread_mutex_lock(&clock_scheduler_events_lock);

    double clock_beat = clock_get_beats();

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        event = &clock_scheduler_events[i];

        if (event->ready && event->type == CLOCK_SCHEDULER_EVENT_SYNC) {
            event->sync_clock_beat = clock_scheduler_next_clock_beat(clock_beat, event->sync_beat, event->sync_beat_offset);
        }
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
}

void clock_scheduler_reset_sync_events() {
    clock_scheduler_event_t *event;

    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        event = &clock_scheduler_events[i];

        if (event->ready && event->type == CLOCK_SCHEDULER_EVENT_SYNC) {
            event->sync_clock_beat = 0;
        }
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
}
