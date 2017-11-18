/*
 * timer.c
 *
 * accurate timers using pthreads and clock_nanosleep.
 */

// std
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// posix / linux
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

// norns
#include "events.h"
#include "timers.h"

#define MAX_NUM_TIMERS_OK 32

enum {
    TIMER_STATUS_RUNNING,
    TIMER_STATUS_STOPPED
};

const int MAX_NUM_TIMERS = MAX_NUM_TIMERS_OK;
struct timer {
    int idx;                     // timer index
    int status;                  // running/stopped status
    double seconds;              // period in seconds
    uint64_t count;              // total iterations ( <=0 -> infinite )
    uint64_t stage;              // current count of iterations
    uint64_t time;               // current time (in nsec)
    uint64_t delta;              // current delta (in nsec)
    pthread_t tid;               // thread id
    pthread_mutex_t stage_lock;  // mutex protecting stage number
    pthread_mutex_t status_lock; // mutex protecting status
};

struct timer timers[MAX_NUM_TIMERS_OK];

//---------------------------
//---- static declarations

static void timer_handle_error(int code, const char *msg) {
    printf("error code: %d ; message: \"%s\"", code, msg); fflush(stdout);
}

static void timer_init(struct timer *t, uint64_t nsec, int count);
static void timer_set_current_time(struct timer *t);
static void *timer_thread_loop(void *timer);
static void timer_bang(struct timer *t);
static void timer_sleep(struct timer *t);
static void timer_reset(struct timer *t, int stage);
static void timer_cancel(struct timer *t);

//------------------------
//---- extern definitions

void timers_init(void) {
    for(int i = 0; i < MAX_NUM_TIMERS_OK; i++) {
        timers[i].status = TIMER_STATUS_STOPPED;
        timers[i].seconds = 1.0;
    }
}

void timer_start(int idx, double seconds, int count, int stage) {
    uint64_t nsec;
    struct timer *t = &timers[idx];

    if( (idx >= 0) && (idx < MAX_NUM_TIMERS_OK) ) {
        pthread_mutex_lock(&t->status_lock);
        if(t->status == TIMER_STATUS_RUNNING) {
            timer_cancel(t);
        }
        pthread_mutex_unlock(&t->status_lock);
        if(seconds > 0.0) {
            timers[idx].seconds = seconds;
        }
        nsec = (uint64_t)(timers[idx].seconds * 1000000000.0);
        timers[idx].idx = idx;
        timer_reset(&timers[idx], stage);
        timer_init(&timers[idx], nsec, count);
    } else {
        printf("invalid timer index, not added. max count of timers is %d\n",
               MAX_NUM_TIMERS_OK);  fflush(stdout);
    }
}

//------------------------
//---- static definitions

static void timer_reset(struct timer *t, int stage) {
  pthread_mutex_lock( &(t->stage_lock) );
  if(stage > 0) { t->stage = stage; }
  else { t->stage = 0; }
  pthread_mutex_unlock( &(t->stage_lock) );
}

void timer_init(struct timer *t, uint64_t nsec, int count) {
    int res;
    pthread_attr_t attr;

    res = pthread_attr_init(&attr);
    if(res != 0) {
        timer_handle_error(res, "pthread_attr_init");
        return;
    }
    // set other thread attributes here...

    t->delta = nsec;
    t->count = count;
    res = pthread_create(&(t->tid), &attr, &timer_thread_loop, (void *)t);
    if(res != 0) {
        timer_handle_error(res, "pthread_create");
        return;
    }
    else {
        t->status = TIMER_STATUS_RUNNING;
        if(res != 0) {
            timer_handle_error(res, "pthread_setschedparam");
            switch(res) {
            case ESRCH:
                printf("specified thread does not exist\n");
		assert(false);
                break;
            case EINVAL:
                printf("invalid thread policy value or associated parameter\n");
		assert(false);
                break;
            case EPERM:
                printf("failed to set scheduling priority.\n");
		// this doesn't need to assert; it can happen with wrong permissions
		// still good for user to know about
                break;
            default:
                printf("unknown error code \n");
		assert(false);
            }
            return;
        }
    }
}

void *timer_thread_loop(void *timer) {
    struct timer *t = (struct timer *) timer;
    int stop = 0;

    timer_set_current_time(t);

    while(!stop) {
      pthread_mutex_lock( &(t->stage_lock) );
      if( ( t->stage >= t->count) && ( t->count > 0) ) {
	stop = 1;
      }
      pthread_mutex_unlock( &(t->stage_lock) );

      if(stop) { break; }
      pthread_testcancel();      

      pthread_mutex_lock( &(t->stage_lock) );
      timer_bang(t);
      t->stage += 1;
      pthread_mutex_unlock( &(t->stage_lock) );      
      timer_sleep(t);
    }
    return NULL;
}

void timer_set_current_time(struct timer *t) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    t->time =
        (uint64_t) ( (1000000000 *
                      (int64_t)time.tv_sec) + (int64_t)time.tv_nsec );
}

void timer_bang(struct timer *t) {
    union event_data *ev = event_data_new(EVENT_TIMER);
    ev->timer.id = t->idx;
    ev->timer.stage = t->stage;
    event_post(ev);
}

void timer_sleep(struct timer *t) {
    struct timespec ts;
    t->time += t->delta;
    ts.tv_sec = t->time / 1000000000;
    ts.tv_nsec = t->time % 1000000000;
    clock_nanosleep (CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
}

void timer_wait(int idx) {
    pthread_join(timers[idx].tid, NULL);
}

void timer_stop(int idx) {
    if( (idx >= 0) && (idx < MAX_NUM_TIMERS_OK) ) {
        pthread_mutex_lock( &(timers[idx].status_lock) );
        if( timers[idx].status == TIMER_STATUS_STOPPED) {
            printf("timer is already stopped\n"); fflush(stdout);
            ;; // nothing to do
        } else {
            printf("cancelling timer\n"); fflush(stdout);
            timer_cancel(&timers[idx]);
        }
        pthread_mutex_unlock( &(timers[idx].status_lock) );
    } else {
        printf("timer_stop(): invalid timer index, max count of timers is %d\n",
               MAX_NUM_TIMERS_OK );  fflush(stdout);
    }
}

void timer_cancel(struct timer *t) {
    int ret = pthread_cancel(t->tid);
    if(ret) {
        printf("timer_stop(): pthread_cancel() failed; error: 0x%08x\r\n", ret);
        fflush(stdout);
    } else {
        t->status = TIMER_STATUS_STOPPED;
    }
}

void timer_set_time(int idx, float sec) {
  if( (idx >= 0) && (idx < MAX_NUM_TIMERS_OK) ) {
    timers[idx].seconds = sec;
    timers[idx].delta = (uint64_t) sec * 1000000000.0;
  }
}

#undef MAX_NUM_TIMERS_OK
