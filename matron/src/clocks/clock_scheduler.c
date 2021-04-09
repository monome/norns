#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "../clock.h"
#include "../events.h"

#include "clock_scheduler.h"

static pthread_t clock_scheduler_tick_thread;
static pthread_mutex_t clock_scheduler_events_lock;

typedef enum {
    CLOCK_SCHEDULER_EVENT_SYNC,
    CLOCK_SCHEDULER_EVENT_SLEEP,
} clock_scheduler_event_type_t;

typedef struct {
    clock_scheduler_event_type_t type;
    int thread_id;

    double sync_beat;
    double sync_beat_clock;

    double sleep_seconds;
    double sleep_seconds_clock;
} clock_scheduler_event_t;

static clock_scheduler_event_t clock_scheduler_events[NUM_CLOCK_SCHEDULER_EVENTS];

static void clock_scheduler_post_clock_resume_event(int thread_id) {
    union event_data *ev = event_data_new(EVENT_CLOCK_RESUME);
    ev->clock_resume.thread_id = thread_id;
    event_post(ev);
}

static void *clock_scheduler_tick_thread_run(void *p) {
    (void)p;
    clock_scheduler_event_t *scheduler_event;
    double clock_beats;
    double clock_seconds;

    while (true) {
        clock_beats = clock_gettime_beats();
        clock_seconds = clock_gettime_secondsf();

        pthread_mutex_lock(&clock_scheduler_events_lock);

        for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
            scheduler_event = &clock_scheduler_events[i];

            if (scheduler_event->thread_id > -1) {
                if (scheduler_event->type == CLOCK_SCHEDULER_EVENT_SYNC) {
                    if (clock_beats >= scheduler_event->sync_beat_clock) {
                        clock_scheduler_post_clock_resume_event(scheduler_event->thread_id);
                        scheduler_event->thread_id = -1;
                    }
                } else {
                    if (clock_seconds >= scheduler_event->sleep_seconds_clock) {
                        clock_scheduler_post_clock_resume_event(scheduler_event->thread_id);
                        scheduler_event->thread_id = -1;
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

bool clock_scheduler_schedule_sync(int thread_id, double beat) {
    double clock_beats = clock_gettime_beats();

    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        if (clock_scheduler_events[i].thread_id == -1) {
            clock_scheduler_events[i].thread_id = thread_id;
            clock_scheduler_events[i].type = CLOCK_SCHEDULER_EVENT_SYNC;
            clock_scheduler_events[i].sync_beat = beat;
            clock_scheduler_events[i].sync_beat_clock = ceil(clock_beats / beat) * beat;

            pthread_mutex_unlock(&clock_scheduler_events_lock);
            return true;
        }
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
    return false;
}

bool clock_scheduler_schedule_sleep(int thread_id, double seconds) {
    double clock_seconds = clock_gettime_secondsf();

    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        if (clock_scheduler_events[i].thread_id == -1) {
            clock_scheduler_events[i].thread_id = thread_id;
            clock_scheduler_events[i].type = CLOCK_SCHEDULER_EVENT_SLEEP;
            clock_scheduler_events[i].sleep_seconds = seconds;
            clock_scheduler_events[i].sleep_seconds_clock = clock_seconds + seconds;

            pthread_mutex_unlock(&clock_scheduler_events_lock);
            return true;
        }
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
    return false;
}

void clock_scheduler_cancel(int thread_id) {
    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        if (clock_scheduler_events[i].thread_id == thread_id) {
            clock_scheduler_events[i].thread_id = -1;
        }
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
}

void clock_scheduler_cancel_all() {
    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        clock_scheduler_events[i].thread_id = -1;
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
}

void clock_scheduler_reschedule_sync_events() {
    clock_scheduler_event_t *scheduler_event;
    double clock_beats = clock_gettime_beats();

    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        scheduler_event = &clock_scheduler_events[i];

        if (scheduler_event->thread_id > -1 && scheduler_event->type == CLOCK_SCHEDULER_EVENT_SYNC) {
            scheduler_event->sync_beat_clock =
                ceil(clock_beats / scheduler_event->sync_beat) * scheduler_event->sync_beat;
        }
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
}
