#include <stdint.h>
#include <stdlib.h>

#include "jack/jack.h"

struct _jack_client {};
struct _jack_port {};

jack_client_t *jack_client_open(const char *client_name, int options, void *status) {
    (void)client_name;
    (void)options;
    (void)status;
    static struct _jack_client dummy;
    return &dummy;
}

int jack_set_xrun_callback(jack_client_t *client, JackXRunCallback callback, void *arg) {
    (void)client;
    (void)callback;
    (void)arg;
    return 0;
}

int jack_activate(jack_client_t *client) {
    (void)client;
    return 0;
}

void jack_client_close(jack_client_t *client) {
    (void)client;
}

float jack_cpu_load(jack_client_t *client) {
    (void)client;
    return 0.0f;
}

jack_nframes_t jack_get_sample_rate(jack_client_t *client) {
    (void)client;
    return (jack_nframes_t)48000u;
}

const char **jack_get_ports(jack_client_t *client, const char *port_name_pattern, const char *type_name_pattern, unsigned long flags) {
    (void)client;
    (void)port_name_pattern;
    (void)type_name_pattern;
    (void)flags;
    return NULL;
}

const jack_port_t *jack_port_by_name(jack_client_t *client, const char *port_name) {
    (void)client;
    (void)port_name;
    return NULL;
}

const char **jack_port_get_all_connections(jack_client_t *client, const jack_port_t *port) {
    (void)client;
    (void)port;
    return NULL;
}

int jack_connect(jack_client_t *client, const char *source_port, const char *destination_port) {
    (void)client;
    (void)source_port;
    (void)destination_port;
    return 0;
}

int jack_disconnect(jack_client_t *client, const char *source_port, const char *destination_port) {
    (void)client;
    (void)source_port;
    (void)destination_port;
    return 0;
}

void jack_free(void *ptr) {
    (void)ptr;
}
