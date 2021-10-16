#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define WATCH_TIME 1

static int count = 0;
static int stage = 0;

static pthread_t p;

void *watch_time(void *);


/* main *************************************/

int main(void) {
  printf("watcher\n");

  if(access("/boot/norns.txt", F_OK)==0) {
    int r = system("echo 'we:sleep' | sudo chpasswd");
    r |= system("sudo rm /boot/norns.txt");
    fprintf(stderr, "password reset: %d\n", r);
  }

  int fd;
  int open_attempts = 0, ioctl_attempts = 0;
  while (open_attempts < 200) {
    fd = open("/dev/input/by-path/platform-keys-event", O_RDONLY);
    if (fd > 0) {
      if (ioctl(fd, EVIOCGRAB, 1) == 0) {
        ioctl(fd, EVIOCGRAB, (void *)0);
        goto done;
      }
      ioctl_attempts++;
      close(fd);
    }
    open_attempts++;
    usleep(50000); // 50ms sleep * 200 = 10s fail after 10s
  };
done:
  if (open_attempts > 0) {
    fprintf(stderr, "required %d open attempts & %d ioctl attempts\n", open_attempts, ioctl_attempts);
  }

  if (fd<1) {
    // didn't find keys
    exit(1);
  }

  if (pthread_create(&p, NULL, watch_time, 0)) {
    fprintf(stderr, "WATCH: Error creating thread\n");
  }

  int n;
  int z;
  int rd;
  struct input_event event[64];
  unsigned int i;

  while (1) {
    rd = read(fd, event, sizeof(struct input_event) * 64); // blocking
    if (rd < (int)sizeof(struct input_event)) {
      fprintf(stderr, "ERROR (key) read error\n");
    }

    for (i = 0; i < rd / sizeof(struct input_event); i++) {
      if (event[i].type) { // make sure it's not EV_SYN == 0
        n = event[i].code;
        z = event[i].value;
        //fprintf(stderr, "%d = %d\n", event[i].code, event[i].value);

        if (z == 0) {
          stage = 0;
        } else if (stage == 0 && n == 3) {
          stage = 1;
        } else if (stage == 1 && n == 2) {
          stage = 2;
        } else if (stage == 2 && n == 1) {
          stage = 3;
          count = 0;
        }
      }
    }
  }

  pthread_cancel(p);
}


void *watch_time(void *x) {
  (void)x;

  while (1) {
    if (stage == 3)
      count++;
    if (count == 10) {
      stage = 0;
      count = 0;
      int res;
      fprintf(stderr, "watcher: restarting norns services...\n");
      res = system("nohup systemctl restart norns-sclang > /dev/null");
      res |= system("nohup systemctl restart norns-crone > /dev/null");
      res |= system("nohup systemctl restart norns-matron > /dev/null");
      fprintf(stderr, "result: %d\n", res);
    }
    sleep(WATCH_TIME);
  }
}


