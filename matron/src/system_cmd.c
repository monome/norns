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


#if 1 // nice big buffers (allocated in worker thread)
static const size_t CMD_CAPTURE_BYTES = 8192 * 8;
static const size_t CMD_LINE_BYTES = 1024;

#else // test with stupid tiny buffers
static const size_t CMD_CAPTURE_BYTES = 128;
static const size_t CMD_LINE_BYTES = 64;
#endif

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
    capture[0] = '\0';
    
    char *line = (char*)malloc(CMD_LINE_BYTES);
    int capacity = CMD_CAPTURE_BYTES - 1;
    
    do {
	// "fgets() reads in at most one less than _size_ characters"
	// so `line` is always null-terminated after these next 2 calls:
	memset(line, '\0', CMD_LINE_BYTES);
	// stop on EOF....
	if (fgets(line, CMD_LINE_CHARS, f) == NULL) { break; }
	
	// we want to skip adding the line if it's incomplete,
	// cause that would bork later
	int len = strlen(line) * sizeof(char);
	if (capacity >= len) {
	    // "if _src_ contains _n_ or more bytes,
	    // strncat() writes _n+1_ bytes to _dest_"
	    // which is why we initialize capacity with -1
	    strncat(capture, line, capacity);
	}
	capacity -= len;
	
#if 0 // test..
	fprintf(stderr, "last line: \n\t%s\n", line);
	fprintf(stderr, "current buffer: \n\t %s\n", line);
	fprintf(stderr, "remaining capacity: %d bytes\n", capacity);
#endif
	
    } while (capacity > 0); // ... or, stop if buffer is full
    
#if 0 // test...
    fprintf(stderr, "finished line loop; full buffer: \n");    
    fprintf(stderr, "%s\n", capture);
#endif

    // just use memcpy and include the null terminator
    size_t len = strlen(capture) + 1;
    char *cap = malloc(len);
    memcpy(cap, capture, len);
    union event_data *ev = event_data_new(EVENT_SYSTEM_CMD);

    // this will get freed when the event is handled
    ev->system_cmd.capture = cap;
    event_post(ev);
    
    free(line);
    free(capture);
    pclose(f);
    
    return NULL;
}
