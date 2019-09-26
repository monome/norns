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
    fprintf(stderr, "system_cmd: error creating thread\n");
  }
}

void *run_cmd(void *cmd) {
  f = popen((char *)cmd, "r");
  if(f == NULL) {
    fprintf(stderr, "system_cmd: command failed\n");
  } else {
    capture[0]=0;
    while (fgets(line, 254, f) != NULL) {
      strcat(capture,line);
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
  pthread_cancel(p);
  return 0;
}
