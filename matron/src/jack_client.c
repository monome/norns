#include <stdatomic.h>
#include <stdio.h>

#include <jack/jack.h>

#include "jack_client.h"

static jack_client_t *jack_client;
double jack_sample_rate;

_Atomic uint32_t xrun_count = 0;

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
    return (double)jack_frame_time(jack_client) / jack_sample_rate;
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
