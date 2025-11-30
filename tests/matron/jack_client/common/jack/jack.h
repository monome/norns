// minimal jack header shim for unit tests.
// this stub satisfies the compiler without pulling in the real jack dependency.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t jack_nframes_t;

typedef struct _jack_client jack_client_t;
typedef struct _jack_port jack_port_t;

// client open flags
enum { JackNoStartServer = 0 };

// port flags used in queries
enum { JackPortIsInput = 1,
       JackPortIsOutput = 2 };

// function pointer types (simplified)
typedef int (*JackXRunCallback)(void *arg);

// minimal prototypes referenced by jack_client.c
jack_client_t *jack_client_open(const char *client_name, int options, void *status);
int jack_set_xrun_callback(jack_client_t *client, JackXRunCallback callback, void *arg);
int jack_activate(jack_client_t *client);
void jack_client_close(jack_client_t *client);
float jack_cpu_load(jack_client_t *client);
jack_nframes_t jack_get_sample_rate(jack_client_t *client);
const char **jack_get_ports(jack_client_t *client, const char *port_name_pattern, const char *type_name_pattern, unsigned long flags);
const jack_port_t *jack_port_by_name(jack_client_t *client, const char *port_name);
const char **jack_port_get_all_connections(jack_client_t *client, const jack_port_t *port);
int jack_connect(jack_client_t *client, const char *source_port, const char *destination_port);
int jack_disconnect(jack_client_t *client, const char *source_port, const char *destination_port);
void jack_free(void *ptr);

// current frame time, defined in the test file
jack_nframes_t jack_frame_time(jack_client_t *client);

#ifdef __cplusplus
}
#endif
