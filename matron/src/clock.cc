#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include "clocks/clock_crow.h"
#include "clocks/clock_internal.h"
#include "clocks/clock_link.h"
#include "clocks/clock_midi.h"
#include "clocks/clock_scheduler.h"
#include "events.h"
#include "jack_client.h"

static clock_source_t clock_source;

void clock_init() {
  clock_set_source(CLOCK_SOURCE_INTERNAL);
}

void clock_reference_init(clock_reference_t *reference) {
    pthread_mutex_init(&(reference->lock), NULL);
    clock_update_source_reference(reference, 0, 0.5);
}

double clock_get_system_time() {
    return jack_client_get_current_time();
}

double clock_get_beats() {
    double beat;

    switch (clock_source) {
    case CLOCK_SOURCE_INTERNAL:
        beat = clock_internal_get_beat();
        break;
    case CLOCK_SOURCE_MIDI:
        beat = clock_midi_get_beat();
        break;
    case CLOCK_SOURCE_LINK:
        beat = clock_link_get_beat();
        break;
    case CLOCK_SOURCE_CROW:
        beat = clock_crow_get_beat();
        break;
    default:
        beat = 0;
        break;
    }

    return beat;
}

double clock_get_reference_beat(clock_reference_t *reference) {
    pthread_mutex_lock(&(reference->lock));

    double current_time = clock_get_system_time();
    double beat = reference->beat + ((current_time - reference->last_beat_time) / reference->beat_duration);

    pthread_mutex_unlock(&(reference->lock));

    return beat;
}

double clock_get_tempo() {
    double tempo;

    switch (clock_source) {
    case CLOCK_SOURCE_INTERNAL:
        tempo = clock_internal_get_tempo();
        break;
    case CLOCK_SOURCE_MIDI:
        tempo = clock_midi_get_tempo();
        break;
    case CLOCK_SOURCE_LINK:
        tempo = clock_link_get_tempo();
        break;
    case CLOCK_SOURCE_CROW:
        tempo = clock_crow_get_tempo();
        break;
    default:
        tempo = 0;
        break;
    }

    return tempo;
}

double clock_get_reference_tempo(clock_reference_t *reference) {
    pthread_mutex_lock(&(reference->lock));

    double tempo = 60.0 / reference->beat_duration;

    pthread_mutex_unlock(&(reference->lock));

    return tempo;
}

void clock_update_source_reference(clock_reference_t *reference, double beat, double beat_duration) {
    pthread_mutex_lock(&(reference->lock));

    double current_time = clock_get_system_time();

    reference->beat_duration = beat_duration;
    reference->last_beat_time = current_time;
    reference->beat = beat;

    pthread_mutex_unlock(&(reference->lock));
}

void clock_start_from_source(clock_source_t source) {
    if (clock_source == source) {
        clock_scheduler_reset_sync_events();
        union event_data *ev = event_data_new(EVENT_CLOCK_START);
        event_post(ev);
    }
}

void clock_stop_from_source(clock_source_t source) {
    if (clock_source == source) {
        union event_data *ev = event_data_new(EVENT_CLOCK_STOP);
        event_post(ev);
    }
}

void clock_reschedule_sync_events_from_source(clock_source_t source) {
    if (clock_source == source) {
        clock_scheduler_reschedule_sync_events();
    }
}

void clock_set_source(clock_source_t source) {
    if (clock_source != source && source == CLOCK_SOURCE_LINK) {
        clock_link_join_session();
    } else if (clock_source != source && clock_source == CLOCK_SOURCE_LINK) {
        clock_link_leave_session();
    }

    clock_source = source;
    clock_scheduler_reschedule_sync_events();
}
