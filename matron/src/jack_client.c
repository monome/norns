#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <jack/jack.h>

#include "jack_client.h"

static jack_client_t *jack_client;
double jack_sample_rate;

_Atomic uint32_t xrun_count = 0;

// maintain a 64-bit frame counter from jack's 32-bit frame time.
// extends wraparound at 48khz from ~25 hours to millions of years.
static uint64_t g_last_total_frames = 0ULL;

// protects access to g_last_total_frames.
// a mutex is safe here as time is not read from the audio thread.
static pthread_mutex_t g_time_lock = PTHREAD_MUTEX_INITIALIZER;

static int xrun_callback(void *arg) {
    (void)arg;
    atomic_fetch_add(&xrun_count, 1);
    return 0;
}

int jack_client_init() {
    jack_client = jack_client_open("matron-clock", JackNoStartServer, NULL);
    if (jack_client == NULL) {
        fprintf(stderr, "failed to create JACK client\n");
        goto fail;
    }

    jack_set_xrun_callback(jack_client, &xrun_callback, 0);

    if (jack_activate(jack_client)) {
        fprintf(stderr, "failed to activate JACK client");
        goto fail;
    }

    jack_sample_rate = (float)jack_get_sample_rate(jack_client);
    return 0;
fail:
    return 1;
}

void jack_client_deinit() {
    jack_client_close(jack_client);
}

float jack_client_get_cpu_load() {
    return jack_cpu_load(jack_client);
}

uint32_t jack_client_get_xrun_count() {
    uint32_t count = atomic_exchange(&xrun_count, 0);
    return count;
}

double jack_client_get_current_time() {
    uint32_t current_frames = (uint32_t)jack_frame_time(jack_client);

    pthread_mutex_lock(&g_time_lock);

    uint32_t last_frames_32 = (uint32_t)g_last_total_frames;

    // calculate delta using modulo arithmetic properties
    int32_t delta = (int32_t)(current_frames - last_frames_32);

    // only advance if delta is positive.
    // ignores backward jitter or extremely long stalls (>12h).
    if (delta > 0) {
        g_last_total_frames += (uint64_t)delta;
    }

    uint64_t result = g_last_total_frames;

    pthread_mutex_unlock(&g_time_lock);

    return (double)result / jack_sample_rate;
}

const char **jack_client_get_input_ports() {
    return jack_get_ports(jack_client, NULL, NULL, JackPortIsInput);
}

const char **jack_client_get_output_ports() {
    return jack_get_ports(jack_client, NULL, NULL, JackPortIsOutput);
}

const char **jack_client_get_port_connections(const char *port_name) {
    const jack_port_t *port = jack_port_by_name(jack_client, port_name);
    if (port != NULL) {
        return jack_port_get_all_connections(jack_client, port);
    }
    return NULL;
}

bool jack_client_connect(const char *source_name, const char *destination_name) {
    return jack_connect(jack_client, source_name, destination_name) == 0;
}

bool jack_client_disconnect(const char *source_name, const char *destination_name) {
    return jack_disconnect(jack_client, source_name, destination_name) == 0;
}

void jack_client_free_port_list(const char **list) {
    jack_free(list);
}

#ifdef NORNS_TEST
void jack_client_test_set_time_state(uint64_t frames) {
    pthread_mutex_lock(&g_time_lock);
    g_last_total_frames = frames;
    pthread_mutex_unlock(&g_time_lock);
}

void jack_client_test_reset_time_state(void) {
    jack_client_test_set_time_state(0ULL);
}
#endif // NORNS_TEST