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
#include <sys/epoll.h>
#include <pthread.h>
#include <linux/input.h>

#include "events.h"

#define NUM_PINS 3

int epfd;
int fd[NUM_PINS];
char buf;
struct epoll_event ev[NUM_PINS];
struct epoll_event events;
pthread_t p;

int enc_fd[3];
pthread_t enc_p[3];

void *gpio_check(void *);
void *enc_check(void *);

// extern def

void gpio_init() {
    epfd = epoll_create(1);
    fd[0] = open("/sys/class/gpio/gpio31/value", O_RDONLY | O_NONBLOCK); // K1
    fd[1] = open("/sys/class/gpio/gpio35/value", O_RDONLY | O_NONBLOCK); // K2
    fd[2] = open("/sys/class/gpio/gpio39/value", O_RDONLY | O_NONBLOCK); // K3
    if(fd[0] > 0) {
        buf = 0;
        int n;
        for(int i = 0; i < NUM_PINS; i++) {
            ev[i].events = EPOLLET | EPOLLIN;
            ev[i].data.fd = fd[i];
            n = epoll_ctl(epfd, EPOLL_CTL_ADD, fd[i], &ev[i]);
        }
        if(n) {
            fprintf(stderr, "ERROR (gpio) epoll_ctl returned %d: %s\n", n,
                    strerror(errno));
        }
        if( pthread_create(&p, NULL, gpio_check, 0) ) {
            fprintf(stderr, "ERROR (gpio) pthread error\n");
        }
    }
    else {
        fprintf(stderr, "ERROR (gpio) check gpio exports!\n");
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

    while(1) {
        rd = read(enc_fd[n], event, sizeof(struct input_event) * 64);
        if(rd < (int) sizeof(struct input_event)) {
            fprintf(stderr, "ERROR (enc) read error\n");
        }

        for(i=0;i<rd/sizeof(struct input_event);i++) {
            if(event[i].type) { // make sure it's not EV_SYN == 0
                //fprintf(stderr, "enc%d = %d\n", n, event[i].value);
                union event_data *ev = event_data_new(EVENT_ENC);
                ev->enc.n = n + 1;
                ev->enc.delta = event[i].value;
                event_post(ev);
            }
        }
    } 
}

void *gpio_check(void *x) {
    (void)x;
    int n;
    unsigned int c[NUM_PINS] = {0,0,0};
    while(1) {
        n = epoll_wait(epfd, &events, 1, -1); // wait for 1 event
        // fprintf(stderr, "GPIO epoll returned %d: %s\n", n, strerror(errno));
        // fprintf(stderr, "GPIO event: %d\n", events.data.fd);

        if(n > 0) {
            n = lseek(events.data.fd, 0, SEEK_SET);
            // fprintf(stderr, "GPIO seek %d bytes: %s\n", n, strerror(errno));
            n = read(events.data.fd, &buf, 1);
            // fprintf(stderr, "GPIO read %d bytes: %s\n", n, strerror(errno));
            // fprintf(stderr, "GPIO buf = 0x%x\n", buf);

            int i;
            for(i = 0; i < NUM_PINS && fd[i] != events.data.fd; i++) {; }

            if(i < 3) {
                if(c[i]++) {
                    union event_data *ev = event_data_new(EVENT_KEY);
                    ev->key.n = i + 1;
                    ev->key.val = !(buf & 0x1);
                    event_post(ev);
                }
            }
/*            else {
                i = i - 3;
                enc_val[i] = (buf & 0x1);

                int a = i >> 1;
                pos_now[a] = enc_val[a << 1] + (enc_val[(a << 1) + 1] << 1);
                int d = map[pos_old[a]][pos_now[a]];

                // fprintf(stderr, "i=%d a=%d e=%d%d d=%d\n", i, a, enc_val[a<<1],
                // enc_val[(a<<1)+1], d);

                if(pos_now[a] != pos_old[a]) {
                    if(d && c[i + 3]++) {
                        union event_data *ev = event_data_new(EVENT_ENC);
                        ev->enc.n = a + 1;
                        ev->enc.delta = d;
                        event_post(ev);
                    }
                    pos_old[a] = pos_now[a];
                }
            }*/
        }
    }
}
