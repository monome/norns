#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "events.h"
#include "oracle.h"
#include "screen.h"
#include "weaver.h"

#define LIGHTS 32
#define GRAVITY 2048

// thread id
static pthread_t tid;
// microseconds per frame
static const int tick_us = 5000;
// frames before timeout
static const int timeout_ticks = 2400; // ~12 seconds?

struct {
    double x;
    double y;
    double dx;
    double dy;
    int range;
    int life;
} light[LIGHTS];

struct {
    double x;
    double y;
    double dx;
    double dy;
} center;

struct {
    int x;
    int y;
} black;

static int count = 0;
static int start = 1;

static int timeout = 0;
static int ok = 0;
static volatile int thread_running = 0;

static int norns_hello();
static void *hello_loop(void *);
static void start_thread();

void *hello_loop(void *p) {
    (void)p;

    thread_running = true;

    while (!ok && !timeout) {

        norns_hello(1);
        o_query_startup();

        if (count > timeout_ticks) {
            timeout = 1;
        }

        usleep(tick_us);
        count++;
        // fprintf(stderr, "%d\n", count);
    }

    // fadeout
    while (norns_hello(0)) {
        usleep(tick_us);
    }

    if (timeout) {
        event_post(event_data_new(EVENT_STARTUP_READY_TIMEOUT));
    } else {
        event_post(event_data_new(EVENT_STARTUP_READY_OK));
    }

    thread_running = false;
    return NULL;
}

void start_thread() {
    // start thread
    int res;
    pthread_attr_t attr;

    res = pthread_attr_init(&attr);
    if (res != 0) {
        fprintf(stderr, "error creating pthread attributes\n");
        return;
    }
    res = pthread_create(&tid, &attr, &hello_loop, NULL);
    if (res != 0) {
        fprintf(stderr, "error creating pthread\n");
    }
}

void norns_hello_start() {
    if (thread_running) {
        return;
    }

    srand(time(NULL));
    screen_aa(0);
    screen_line_width(1);

    for (int i = 0; i < LIGHTS; i++) {
        light[i].range = 1;
    }

    black.x = 60 + rand() % 8;
    black.y = 28 + rand() % 8;
    center.x = 60 + rand() % 8;
    center.y = 28 + rand() % 8;
    center.dx = 0;
    center.dy = 0;
    count = 0;

    timeout = 0;
    ok = 0;

    start_thread();
}

void norns_hello_ok() {
    ok = 1;
}

int norns_hello(int live) {

    if ((count & 255) == 0) {
        black.x = 60 + rand() % 8;
        black.y = 28 + rand() % 8;
    }

    screen_clear();
    // screen_line_width(1.0); // FIXME: for some reason setting this disables drawing

    center.dx = center.dx + (black.x - center.x) / GRAVITY;
    center.dy = center.dy + (black.y - center.y) / GRAVITY;
    center.x = center.x + center.dx;
    center.y = center.y + center.dy;
    int alive = 0;

    for (int i = 0; i < LIGHTS; i++) {
        if (light[i].range == 2) {
            if (start < 64)
                start++;
            light[i].range--;
        } else if (light[i].range > 2) {
            light[i].range--;
            light[i].x += light[i].dx;
            light[i].y += light[i].dy;
            alive++;
            screen_rect(light[i].x, light[i].y, 1, 1);
            screen_level(ceil(15 * light[i].range / light[i].life) * (start / 64.0));
            screen_stroke();
        } else if (live) {
            light[i].life = 64 + rand() % 64;
            light[i].range = light[i].life;
            light[i].x = center.x;
            light[i].y = center.y;
            light[i].dx = (rand() % 32 - 16) / 96.0;
            light[i].dy = (rand() % 32 - 16) / 96.0;
        }
    }
    screen_update();

    return alive;
}
