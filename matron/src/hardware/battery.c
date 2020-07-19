/*
 * battery.c
 *
 * bq27441 power supply
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "events.h"

#define BATTERY_POLL_INTERVAL 5

static int fd[3];
static char buf[8];
static pthread_t p;

void *battery_check(void *);

// extern def

void battery_init() {
    fd[0] = open("/sys/class/power_supply/bq27441-0/capacity", O_RDONLY | O_NONBLOCK);
    fd[1] = open("/sys/class/power_supply/bq27441-0/status", O_RDONLY | O_NONBLOCK);
    fd[2] = open("/sys/class/power_supply/bq27441-0/current_now", O_RDONLY | O_NONBLOCK);
    if (fd[0] > 0) {
        if (pthread_create(&p, NULL, battery_check, 0)) {
            fprintf(stderr, "BATTERY: Error creating thread\n");
        }
    } else {
        fprintf(stderr, "BATTERY: FAIL.\n");
    }
}

void battery_deinit() {
    pthread_cancel(p);
}

void *battery_check(void *x) {
    (void)x;
    int n;
    int percent = -1; // use -1 to trigger first reading
    int present = -1;
    int current = -1;

    while (1) {
        lseek(fd[0], 0, SEEK_SET);
        if (read(fd[0], &buf, 4) > 0) {
            percent = atoi(buf);
        } else {
            // fprintf(stderr, "failed to read battery percentage\n");
            percent = -1;
        }

        lseek(fd[1], 0, SEEK_SET);
        if (read(fd[1], &buf, 1) > 0) {
            n = (buf[0] == 'C'); // Charging
        } else {
            // fprintf(stderr, "failed to read battery charging status\n");
            n = 0;
        }

        if (n != present) {
            present = n;
            union event_data *ev = event_data_new(EVENT_POWER);
            ev->power.present = present;
            event_post(ev);
        }

        lseek(fd[2], 0, SEEK_SET);
        if (read(fd[2], &buf, 8) > 0) {
            n = atoi(buf) / 1000;
        } else {
            // fprintf(stderr, "failed to read battery current\n");
            n = -1;
        }

        if (n != current) {
            current = n;
            union event_data *ev = event_data_new(EVENT_BATTERY);
            ev->battery.percent = percent;
            ev->battery.current = current;
            event_post(ev);
        }

        sleep(BATTERY_POLL_INTERVAL);
    }
}
