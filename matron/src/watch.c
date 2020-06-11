/*
 * watch.c
 *
 * tracks key state, resets after timer
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "events.h"

#define WATCH_TIME 1

static int count = 0;
static int stage = 0;

static pthread_t p;

void *watch_time(void *);

// extern def

void watch_init() {
    if (pthread_create(&p, NULL, watch_time, 0)) {
        fprintf(stderr, "WATCH: Error creating thread\n");
    }
}

void watch_deinit() {
    pthread_cancel(p);
}

void *watch_time(void *x) {
    (void)x;

    while (1) {
        if (stage == 3)
            count++;
        if (count == 10) {
            fprintf(stderr, "RESTARTING...\n");
            system("nohup systemctl restart norns-sclang > /dev/null");
            system("nohup systemctl restart norns-crone > /dev/null");
            system("nohup systemctl restart norns-matron > /dev/null");
        }
        sleep(WATCH_TIME);
    }
}

void watch_key(int n, int z) {
    if (z == 0) {
        stage = 0;
    } else if (stage == 0 && n == 3) {
        stage = 1;
    } else if (stage == 1 && n == 2) {
        stage = 2;
    } else if (stage == 2 && n == 1) {
        stage = 3;
    }
}
