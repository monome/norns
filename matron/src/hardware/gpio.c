/*
 * gpio.c
 *
 * keys and encoders
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <linux/input.h>
#include <time.h>

#include "events.h"
#include "watch.h"


int key_fd;
pthread_t key_p;

int enc_fd[3];
pthread_t enc_p[3];

void *key_check(void *);
void *enc_check(void *);

// extern def

static int open_and_grab (const char *pathname, int flags) {
    int fd;
    int open_attempts=0, ioctl_attempts=0;
    while (open_attempts < 200) {
        fd = open(pathname, flags);
        if(fd > 0) {
            if(ioctl(fd, EVIOCGRAB, 1) == 0) {
                ioctl(fd, EVIOCGRAB, (void*)0);
                goto done;
            }
            ioctl_attempts++;
            close(fd);
        }
        open_attempts++;
        usleep(50000); // 50ms sleep * 200 = 10s fail after 10s
    };
 done:
    if(open_attempts > 0) {
        fprintf(stderr, "WARN open_and_grab GPIO '%s' required %d open attempts & %d ioctl attempts\n", pathname, open_attempts, ioctl_attempts);
    }
    return fd;
}

void gpio_init() {
    key_fd = open_and_grab("/dev/input/by-path/platform-keys-event", O_RDONLY); // Keys
    if(key_fd > 0) {
        if(pthread_create(&key_p, NULL, key_check, 0) ) {
            fprintf(stderr, "ERROR (keys) pthread error\n");
        }
    }

    char *enc_filenames[3] = {"/dev/input/by-path/platform-soc:knob1-event",
                              "/dev/input/by-path/platform-soc:knob2-event",
                              "/dev/input/by-path/platform-soc:knob3-event"};
    for(int i=0; i < 3; i++) {
        enc_fd[i] = open_and_grab(enc_filenames[i], O_RDONLY);
        if(enc_fd[i] > 0) {
            int *arg = malloc(sizeof(int));
            *arg = i;
            if(pthread_create(&enc_p[i], NULL, enc_check, arg) ) {
                fprintf(stderr, "ERROR (enc%d) pthread error\n",i);
            }
	}
    }
}

void gpio_deinit() {
    pthread_cancel(key_p);
    pthread_cancel(enc_p[0]);
    pthread_cancel(enc_p[1]);
    pthread_cancel(enc_p[2]);
}

void *enc_check(void *x) {
    int n = *((int *)x);
    free(x);
    int rd;
    unsigned int i;
    struct input_event event[64];
    int dir[3] = {1,1,1};
    clock_t now[3];
    clock_t prev[3];
    clock_t diff;
    prev[0] = prev[1] = prev[2] = clock();

    while(1) {
        rd = read(enc_fd[n], event, sizeof(struct input_event) * 64);
        if(rd < (int) sizeof(struct input_event)) {
            fprintf(stderr, "ERROR (enc) read error\n");
        }

        for(i=0;i<rd/sizeof(struct input_event);i++) {
          if(event[i].type) { // make sure it's not EV_SYN == 0
            now[i] = clock();
            diff = now[i] - prev[i];
            //fprintf(stderr, "%d\t%d\t%lu\n", n, event[i].value, diff);
            prev[i] = now[i];
            if(diff > 100) { // filter out glitches
              if(dir[i] != event[i].value && diff > 500) { // only reverse direction if there is reasonable settling time
                dir[i] = event[i].value;
              }
              union event_data *ev = event_data_new(EVENT_ENC);
              ev->enc.n = n + 1;
              ev->enc.delta = event[i].value;
              event_post(ev);
            }
          }
        }
    } 
}

void *key_check(void *x) {
    (void)x;
    int rd;
    unsigned int i;
    struct input_event event[64];

    while(1) {
        rd = read(key_fd, event, sizeof(struct input_event) * 64);
        if(rd < (int) sizeof(struct input_event)) {
            fprintf(stderr, "ERROR (key) read error\n");
        }

        for(i=0;i<rd/sizeof(struct input_event);i++) {
            if(event[i].type) { // make sure it's not EV_SYN == 0
                //fprintf(stderr, "enc%d = %d\n", n, event[i].value);
                union event_data *ev = event_data_new(EVENT_KEY);
                ev->key.n = event[i].code;
                ev->key.val = event[i].value;
                event_post(ev);

                watch_key(event[i].code,event[i].value);
            }
        }
    }
}
