#include <semaphore.h>
#include <stdio.h>

#include "events.h"
#include "screen_results.h"

// instead of a queue, we just have a single event here!
// i think for the moment it is enough, 
// since only one blocking request can be in flight at a time
// note that we only store a pointer, 
// and consumer must free the event data
static union event_data* ev_result;
static sem_t sem_result;

void screen_results_init() {
    sem_init(&sem_result, 0, 0);
}    

void screen_results_wait() {
    fprintf(stderr, "screen_results_wait\n");
    sem_wait(&sem_result);
}

void screen_results_post(union event_data *ev) {
    ev_result = ev;
    fprintf(stderr, "screen_results_post\n");
    sem_post(&sem_result);
}
union event_data *screen_results_get() {
    return ev_result;
}