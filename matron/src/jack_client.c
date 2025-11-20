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
    jack_nframes_t cur_low = jack_frame_time(jack_client);

    const unsigned low_bits = (unsigned)(sizeof(jack_nframes_t) * 8u);
    const uint64_t low_mask = (low_bits == 64u) ? UINT64_MAX : ((1ULL << low_bits) - 1ULL);

    pthread_mutex_lock(&g_time_lock);

    uint64_t prev = g_last_total_frames;
    uint64_t prev_low = prev & low_mask;
    uint64_t prev_high = prev & ~low_mask;

    uint64_t cur_low_u64 = (uint64_t)cur_low;
    uint64_t candidate = prev_high | cur_low_u64;

    // detect wrap and carry to high bits.
    // a wrap is a large decrease (prev in upper half, cur in lower).
    if (low_bits < 64u && cur_low_u64 < prev_low) {
        const uint64_t half = (1ULL << (low_bits - 1));
        if (prev_low >= half && cur_low_u64 < half) {
            candidate += (1ULL << low_bits);
        } else {
            // ignore small decreases (out-of-order reads)
            candidate = prev;
        }
    }

    // never move backwards
    if (candidate < prev) {
        candidate = prev;
    }

    g_last_total_frames = candidate;

    pthread_mutex_unlock(&g_time_lock);

    return (double)candidate / jack_sample_rate;
}

#ifdef NORNS_TEST
void jack_client_test_reset_time_state(void) {
    pthread_mutex_lock(&g_time_lock);
    g_last_total_frames = 0ULL;
    pthread_mutex_unlock(&g_time_lock);
}
#endif

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
