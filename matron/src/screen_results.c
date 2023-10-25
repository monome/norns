#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "events.h"
#include "screen_results.h"

// instead of a queue, we just have a single event here!
// i think for the moment it is enough, 
// since only one blocking request can be in flight at a time
static union screen_results_data* results_data;
static sem_t sem_results;

void screen_results_init() {
    results_data = NULL;
    sem_init(&sem_results, 0, 0);
}    

void screen_results_deinit() {
    screen_results_free();
}    

void screen_results_wait() {
    sem_wait(&sem_results);
}

void screen_results_post(union screen_results_data *data) {
    if (results_data != NULL) { 
        // the old data hasn't been handled yet!
        // this shouldn't happen
        fprintf(stderr, "SYSTEM ERROR: screen_results_post: dropping unhandled results data!\n");
        screen_results_free();
    }
    results_data = data;
    sem_post(&sem_results);
}

union screen_results_data* screen_results_data_new(screen_results_t type) { 
    union screen_results_data *data = calloc(1, sizeof(union screen_results_data));
    data->type = type;
    return data;
}

// only ever frees the single item in the queue
void screen_results_free() {
    if (results_data != NULL) {
        switch(results_data->type) { 
            case SCREEN_RESULTS_TEXT_EXTENTS:
                break;
            case SCREEN_RESULTS_CURRENT_POINT:
                break;
            case SCREEN_RESULTS_PEEK:
                free(results_data->peek.buf);
                break;
        }
        free(results_data);
        results_data = NULL;
    }
}

union screen_results_data *screen_results_get() {
    return results_data;
}