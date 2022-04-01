/*
 * system_cmd.c
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "events.h"
#include "sidecar.h"

void *run_cmd(void *);

// extern def
void system_cmd(char *cmd) {
    pthread_t p;
    fprintf(stderr, "system_cmd: %s\n", cmd);
    if (pthread_create(&p, NULL, run_cmd, cmd)) {
        fprintf(stderr, "system_cmd: error in pthread_create() \n");
    }
    pthread_detach(p);
}

void *run_cmd(void *cmd) {
    size_t size=0;
    char *buff = NULL;
    sidecar_client_cmd(&buff, &size, (char *)cmd);

    if (size==0) {
        fprintf(stderr, "system_cmd: command failed\n");
        return NULL;
    }

    union event_data *ev = event_data_new(EVENT_SYSTEM_CMD);
    // this will get freed when the event is handled
    ev->system_cmd.capture = buff;
    event_post(ev);

    return NULL;
}
