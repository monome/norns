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

static const size_t CMD_CAPTURE_BYTES = 8192 * 8;
static const size_t CMD_LINE_BYTES = 1024;

void *run_cmd(void *);

// extern def

void system_cmd(char *cmd) {
    pthread_t p;
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
    const size_t CMD_LINE_CHARS = CMD_LINE_BYTES / sizeof(char);
    
    FILE *f = popen((char *)cmd, "r");
    if (f == NULL) {
        fprintf(stderr, "system_cmd: command failed\n");
	return NULL;
    }
	
    char *capture = (char*)malloc(CMD_CAPTURE_BYTES);
    char *line = (char*)malloc(CMD_LINE_BYTES);
    size_t capacity = CMD_CAPTURE_BYTES - 1;
    
    do {
	// "fgets() reads in at most one less than _size_ characters"
	// so `line` is always null-terminated after these next 2 calls:
	memset(line, '\0', CMD_LINE_BYTES);
	// stop on EOF....
	if (fgets(line, CMD_LINE_CHARS, f)) { break; }
	// "if _src_ contains _n_ or more bytes,
	// strncat() writes _n+1_ bytes to _dest_"
	// which is why we initialize capacity with -1
	strncat(capture, line, capacity);
	capacity -= strlen(line) * sizeof(char);
    } while (capacity > 0); // ...or, stop if buffer is full

    size_t len = strlen(capture);
    char *cap = malloc((len + 1) * sizeof(char));
    strncpy(cap, capture, len);
    cap[len] = '\0';
    union event_data *ev = event_data_new(EVENT_SYSTEM_CMD);

    // this will get freed when the event is handled
    ev->system_cmd.capture = cap;
    event_post(ev);
    
    free(line);
    free(capture);
    pclose(f);
    
    return NULL;
}
