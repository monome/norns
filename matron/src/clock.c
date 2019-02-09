#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#include "events.h"
#include "clock.h"

#include <lua.h>
#include <lauxlib.h>

#define NUM_THREADS 10

static void *clock_loop(void *p);

static pthread_t clock_threads[NUM_THREADS];
static int thread_count = 0;

struct thread_arg {
    lua_State *thread_state;
    int thread_ref;
};

static void *clock_loop(void *p) {
    struct lua_State *thread_state = p;
    struct timespec req;

    while (true) {
        int resume_result = lua_resume(thread_state, NULL, 0);

        if (resume_result == LUA_YIELD) {
            float seconds = luaL_checknumber(thread_state, 1);

            uint64_t nsec = (uint64_t) (seconds * 1000000000.0);

            req.tv_sec = nsec / 1000000000;
            req.tv_nsec = nsec % 1000000000;

            nanosleep(&req, NULL);
        } else {
            break;
        }

        // union event_data *ev = event_data_new(EVENT_CLOCK_RESUME);
        // ev->clock.id = 0;
        // ev->clock.stage = 0;
        // event_post(ev);
    }

    return NULL;
}

void clock_start(lua_State *thread_state) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    if (thread_count == NUM_THREADS) {
        fprintf(stderr, "sorry, no more threads currently possible\n");
        return;
    }

    pthread_create(&clock_threads[0], &attr, &clock_loop, thread_state);
    thread_count++;
}
