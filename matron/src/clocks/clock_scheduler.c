#include <float.h>
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
    bool ready;
    int thread_id;

    double sync_beat;
    double sync_clock_beat;

    double sleep_time;
    double sleep_clock_time;
} clock_scheduler_event_t;

static clock_scheduler_event_t clock_scheduler_events[NUM_CLOCK_SCHEDULER_EVENTS];

static void clock_scheduler_post_clock_resume_event(int thread_id) {
    union event_data *ev = event_data_new(EVENT_CLOCK_RESUME);
    ev->clock_resume.thread_id = thread_id;
    event_post(ev);
}

static inline double clock_scheduler_next_clock_beat(double clock_beat, double sync_beat) {
    return ceil((clock_beat + FLT_EPSILON) / sync_beat) * sync_beat;
}

static void *clock_scheduler_tick_thread_run(void *p) {
    (void)p;
    clock_scheduler_event_t *scheduler_event;
    double clock_beat;
    double clock_time;

    while (true) {
        clock_time = clock_gettime_seconds();
        clock_beat = clock_gettime_beats();

        pthread_mutex_lock(&clock_scheduler_events_lock);

        for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
            scheduler_event = &clock_scheduler_events[i];

            if (scheduler_event->ready) {
                if (scheduler_event->type == CLOCK_SCHEDULER_EVENT_SYNC) {
                    if (clock_beat > scheduler_event->sync_clock_beat) {
                        clock_scheduler_post_clock_resume_event(scheduler_event->thread_id);
                        scheduler_event->ready = false;
                    } else {
                        if (scheduler_event->sync_clock_beat - clock_beat > scheduler_event->sync_beat) {
                            scheduler_event->sync_clock_beat = clock_scheduler_next_clock_beat(clock_beat, scheduler_event->sync_beat);
                        }
                    }
                } else {
                    if (clock_time >= scheduler_event->sleep_clock_time) {
                        clock_scheduler_post_clock_resume_event(scheduler_event->thread_id);
                        scheduler_event->ready = false;
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

bool clock_scheduler_schedule_sync(int thread_id, double sync_beat) {
    double clock_beat = clock_gettime_beats();

    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        if (clock_scheduler_events[i].thread_id == thread_id) {
            clock_scheduler_events[i].ready = true;
            clock_scheduler_events[i].thread_id = thread_id;
            clock_scheduler_events[i].sync_beat = sync_beat;

            if (clock_scheduler_events[i].type == CLOCK_SCHEDULER_EVENT_SYNC) {
                clock_scheduler_events[i].sync_clock_beat = clock_scheduler_next_clock_beat(clock_scheduler_events[i].sync_clock_beat, sync_beat);
            } else {
                clock_scheduler_events[i].type = CLOCK_SCHEDULER_EVENT_SYNC;
                clock_scheduler_events[i].sync_clock_beat = clock_scheduler_next_clock_beat(clock_beat, sync_beat);
                ;
            }

            pthread_mutex_unlock(&clock_scheduler_events_lock);
            return true;
        }
    }

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        if (clock_scheduler_events[i].thread_id == -1) {
            clock_scheduler_events[i].ready = true;
            clock_scheduler_events[i].thread_id = thread_id;
            clock_scheduler_events[i].sync_beat = sync_beat;
            clock_scheduler_events[i].sync_clock_beat = clock_scheduler_next_clock_beat(clock_beat, sync_beat);
            clock_scheduler_events[i].type = CLOCK_SCHEDULER_EVENT_SYNC;

            pthread_mutex_unlock(&clock_scheduler_events_lock);
            return true;
        }
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
    return false;
}

bool clock_scheduler_schedule_sleep(int thread_id, double seconds) {
    double clock_time = clock_gettime_seconds();

    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        if (clock_scheduler_events[i].thread_id == -1) {
            clock_scheduler_events[i].ready = true;
            clock_scheduler_events[i].thread_id = thread_id;
            clock_scheduler_events[i].type = CLOCK_SCHEDULER_EVENT_SLEEP;
            clock_scheduler_events[i].sleep_time = seconds;
            clock_scheduler_events[i].sleep_clock_time = clock_time + seconds;

            pthread_mutex_unlock(&clock_scheduler_events_lock);
            return true;
        }
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
    clock_scheduler_event_t *scheduler_event;
    double clock_beat = clock_gettime_beats();

    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        scheduler_event = &clock_scheduler_events[i];

        if (scheduler_event->ready && scheduler_event->type == CLOCK_SCHEDULER_EVENT_SYNC) {
            scheduler_event->sync_clock_beat = clock_scheduler_next_clock_beat(clock_beat, scheduler_event->sync_beat);
        }
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
}

void clock_scheduler_reset_sync_events() {
    clock_scheduler_event_t *scheduler_event;

    pthread_mutex_lock(&clock_scheduler_events_lock);

    for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; i++) {
        scheduler_event = &clock_scheduler_events[i];

        if (scheduler_event->ready && scheduler_event->type == CLOCK_SCHEDULER_EVENT_SYNC) {
            scheduler_event->sync_clock_beat = 0;
        }
    }

    pthread_mutex_unlock(&clock_scheduler_events_lock);
}
