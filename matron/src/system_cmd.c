/*
 * system_cmd.c
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include "events.h"

#define CMD_BUFFER 8192

static FILE *f;
static pthread_t p;
static char capture[CMD_BUFFER];
static char line[255];
static int len;

void *run_cmd(void *);

// extern def

void system_cmd(char *cmd) {
    if (pthread_create(&p, NULL, run_cmd, cmd) ) {
        fprintf(stderr, "system_cmd: error in pthread_create() \n");
    }
    // by default, a pthread is created in "joinable" state,
    // meaning it will retain its stack and other resources until joined.
    // we need to detach the thread, either manually (as below)
    // or by specificying the PTHREAD_CREATE_DETACHED attribute
    pthread_detach(p);
}

void *run_cmd(void *cmd) {
    f = popen((char *)cmd, "r");
    if(f == NULL) {
        fprintf(stderr, "system_cmd: command failed\n");
    } else {
        capture[0]=0;
        while (fgets(line, 254, f) != NULL) {
            strcat(capture, line);
        }
        len = strlen(capture);
        char *cap = malloc( (len + 1) * sizeof(char) );
        strncpy(cap, capture, len);
        cap[len] = '\0';
        union event_data *ev = event_data_new(EVENT_SYSTEM_CMD);
        ev->system_cmd.capture = cap;
        event_post(ev);
        //fprintf(stderr, "%s", capture);
        pclose(f);
    }
    return NULL;
}
