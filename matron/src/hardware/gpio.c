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


int fd;
pthread_t p;

int enc_fd[3];
pthread_t enc_p[3];

void *key_check(void *);
void *enc_check(void *);

// extern def

void gpio_init() {
    fd = open("/dev/input/by-path/platform-keys-event", O_RDONLY); // Keys
    if(fd > 0) {
        if(ioctl(fd, EVIOCGRAB, 1) == 0) {
            ioctl(fd, EVIOCGRAB, (void*)0);
        }
        else {
            fprintf(stderr, "ERROR (keys) grab fail\n");
        }

        if(pthread_create(&p, NULL, key_check, 0) ) {
            fprintf(stderr, "ERROR (keys) pthread error\n");
        } 
    }
    else {
        fprintf(stderr, "ERROR (keys) fail!!\n");
    }

    enc_fd[0] = open("/dev/input/by-path/platform-soc:knob1-event", O_RDONLY);
    enc_fd[1] = open("/dev/input/by-path/platform-soc:knob2-event", O_RDONLY);
    enc_fd[2] = open("/dev/input/by-path/platform-soc:knob3-event", O_RDONLY);
    if(enc_fd[0] > 0) {
        for(int i = 0; i< 3; i++) {

            if(ioctl(enc_fd[i], EVIOCGRAB, 1) == 0) {
                ioctl(enc_fd[i], EVIOCGRAB, (void*)0);
            }
            else {
                fprintf(stderr, "ERROR (enc) grab fail\n");
            }
    
            int *arg = malloc(sizeof(int));
            *arg = i; 
            if(pthread_create(&enc_p[i], NULL, enc_check, arg) ) {
                fprintf(stderr, "ERROR (enc) pthread error\n");
            } 
        }
    }
    else {
        fprintf(stderr, "ERROR (enc) didn't work\n");
    } 
}

void gpio_deinit() {
    pthread_cancel(p);
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
        rd = read(fd, event, sizeof(struct input_event) * 64);
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
            }
        }
    }
}
