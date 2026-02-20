#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

#include "events.h"
#include "screen_results.h"

static union screen_results_data *results_data;

#ifdef __APPLE__
static dispatch_semaphore_t sem_results;
#else
static sem_t sem_results;
#endif

void screen_results_init() {
    results_data = NULL;
#ifdef __APPLE__
    sem_results = dispatch_semaphore_create(0);
#else
    sem_init(&sem_results, 0, 0);
#endif
}

void screen_results_deinit() {
    screen_results_free();
}

void screen_results_wait() {
#ifdef __APPLE__
    dispatch_semaphore_wait(sem_results, DISPATCH_TIME_FOREVER);
#else
    sem_wait(&sem_results);
#endif
}

void screen_results_post(union screen_results_data *data) {
    if (results_data != NULL) {
        // the old data hasn't been handled yet!
        // this shouldn't happen
        fprintf(stderr, "SYSTEM ERROR: screen_results_post: dropping unhandled results data!\n");
        screen_results_free();
    }
    results_data = data;
#ifdef __APPLE__
    dispatch_semaphore_signal(sem_results);
#else
    sem_post(&sem_results);
#endif
}

union screen_results_data *screen_results_data_new(screen_results_t type) {
    union screen_results_data *data = calloc(1, sizeof(union screen_results_data));
    data->type = type;
    return data;
}

// only ever frees the single item in the queue
void screen_results_free() {
    if (results_data != NULL) {
        switch (results_data->type) {
        case SCREEN_RESULTS_TEXT_EXTENTS:
            break;
        case SCREEN_RESULTS_CURRENT_POINT:
            break;
        case SCREEN_RESULTS_PEEK:
            free(results_data->peek.buf);
            break;
        case SCREEN_RESULTS_CONTEXT_NEW:
            break;
        case SCREEN_RESULTS_CONTEXT_GET_CURRENT:
            break;
        case SCREEN_RESULTS_SURFACE_GET_EXTENTS:
            break;
        }
        free(results_data);
        results_data = NULL;
    }
}

union screen_results_data *screen_results_get() {
    return results_data;
}