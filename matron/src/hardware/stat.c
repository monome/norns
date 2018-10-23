/*
 * stat.c
 *
 * hardware status monitoring
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#include "events.h"

#define DISK_INTERVAL 10

//static int fd[3];
//static char buf[8];
static pthread_t p;

void *stat_check(void *);

// extern def

void stat_init() {
  if (pthread_create(&p, NULL, stat_check, 0) ) {
    fprintf(stderr, "STAT: Error creating thread\n");
  }
}

void stat_deinit() {
  pthread_cancel(p);
}

void *stat_check(void *x) {
  (void)x;
  //int n;
  //int percent = -1; // use -1 to trigger first reading
  //int present = -1;
  //int current = -1;

	FILE *fd;
	char buf[8];

  while (1) {
		if ((fd = popen("df -hl | grep '/dev/root' | awk '{print $4}'", "r")) == NULL) {
        fprintf(stderr,"Error opening pipe: disk free read\n");
    }
		else {
			while (fgets(buf, 8, fd) != NULL) {
        fprintf(stderr,"disk free: %s", buf);
			}
    }

/*    if (n != current) {
      current = n;
      union event_data *ev = event_data_new(EVENT_BATTERY);
      ev->battery.percent = percent;
      ev->battery.current = current;
      event_post(ev);
    }
*/
    sleep(DISK_INTERVAL);
  }
}
