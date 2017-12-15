/*
 * battery.c
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

#include "events.h"

#define BATTERY_POLL_INTERVAL 5

static int fd[2];
static char buf[3];
static pthread_t p;

void *battery_check(void *);

// extern def

void battery_init() {
    fd[0] = open("/sys/class/power_supply/bq27441-0/capacity",
                 O_RDONLY | O_NONBLOCK); // K1
    fd[1] = open("/sys/class/power_supply/bq27441-0/status",
                 O_RDONLY | O_NONBLOCK); // K2
    if(fd[0] > 0) {
        //printf( "BATTERY: %s\n", strerror(errno) ); fflush(stdout);
        if( pthread_create(&p, NULL, battery_check, 0) ) {
            printf("BATTERY: Error creating thread\n"); fflush(stdout);
        }
    }
    else {
        printf("BATTERY: FAIL.\n"); fflush(stdout);
    }
}

void battery_deinit() {
    pthread_cancel(p);
}

void *battery_check(void *x) {
    (void)x;
    int n;
    int percent;
    int present;

    while(1) {
        lseek(fd[0],0,SEEK_SET);
        read(fd[0],&buf,3);
        n = atoi(buf);
        //printf("BATTERY = %d\n", n); fflush(stdout);
        if(n != percent) {
            percent = n;
            union event_data *ev = event_data_new(EVENT_BATTERY);
            ev->battery.percent = percent;
            event_post(ev);
        }

        lseek(fd[1],0,SEEK_SET);
        read(fd[1],&buf,1);
        n = (buf[0] == 'C');
        //printf("POWER = %d\n", n); fflush(stdout);
        if(n != present) {
            present = n;
            union event_data *ev = event_data_new(EVENT_POWER);
            ev->power.present = present;
            event_post(ev);
        }

        sleep(BATTERY_POLL_INTERVAL);
    }
}
