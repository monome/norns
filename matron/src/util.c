/*
 * util.c
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

static int fd;
static pthread_t p;

void *run_cmd(void *);

// extern def

void system_cmd(char *cmd) {
  if (pthread_create(&p, NULL, run_cmd, cmd) ) {
    fprintf(stderr, "system_cmd: error creating thread\n");
  }
}

void *run_cmd(void *cmd) {
  fd = popen((char *)cmd, "r");
  if(fd) {
    fprintf(stderr, "system_cmd: command failed\n");
  } else {
    fprintf(stderr, "%s", fd);
    pclose(fd);
  }
  pthread_cancel(p);
}
