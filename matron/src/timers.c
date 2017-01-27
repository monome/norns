/*
  timer.c
  
  accurate timers using pthreads and clock_nanosleep.

  TODO: needs thread synchronization to complete the API.
*/

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "timers.h"

#define MAX_NUM_TIMERS_OK 32
const int MAX_NUM_TIMERS = MAX_NUM_TIMERS_OK;

struct timer {
  int idx;         // timer index
  int count;       // count of iterations ( <0 -> infinite )
  uint64_t time;   // current time (in nsec)
  uint64_t delta;  // current delta (in nsec)
  timer_cb cb;     // callback
  pthread_t tid;   // thread id
};

struct timer timers[MAX_NUM_TIMERS_OK];

//---------------------------
//---- static declarations
static void timer_handle_error(int code, const char* msg) {
  printf("error code: %s ; message: \"%s\"", code, msg);
}

static void timer_init(struct timer *t, timer_cb cb, uint64_t nsec, int count);
static void timer_set_current_time(struct timer *t);
static void* timer_thread_start(void* timer);
static void timer_bang(struct timer *t, counter_t* counter);

//------------------------
//---- extern definitions
void timer_add(int idx, timer_cb cb, double seconds, int count) {
  uint64_t nsec = (uint64_t) (seconds * 1000000000.0);
  printf("adding timer at index %d, period %d nsec, count %d \r\n",
		 idx, nsec, count);
  if(idx < MAX_NUM_TIMERS) { 
	timer_init(&timers[idx], cb, nsec, count);
  }			   
}
			   
//------------------------
//---- static definitions

void timer_init(struct timer *t, timer_cb cb, uint64_t nsec, int count) {
  int res;
  pthread_attr_t attr;
  
  res = pthread_attr_init(&attr);
  if(res != 0) { timer_handle_error(res, "pthread_attr_init"); }
  // set other thread attributes here... 

  t->delta = nsec;
  t->cb = cb;
  t->count = count;
  
  res = pthread_create(&(t->tid), &attr, &timer_thread_start, (void*)t);
  if(res !=0) { timer_handle_error(res, "pthread_create"); }
  else {
	// set thread priority to realtime
	struct sched_param param;
	param.sched_priority = sched_get_priority_max (SCHED_RR);
	res = pthread_setschedparam (t->tid, SCHED_RR, &param);
	if(res !=0) { timer_handle_error(res, "pthread_setschedparam"); }
	/* else { */
	/*   printf("created timer; thread id: %d \r\n", t->tid); */
	/* } */
  }
}

void* timer_thread_start(void* timer) {
  struct timer *t = (struct timer*) timer;
  counter_t i = 0;

  timer_set_current_time(t);
  
  if(t->count < 0) {
	while(1) {
	  timer_bang(t, &i);
	}
  } else {
	while(i < (t->count)) {
	  timer_bang(t, &i);
	}
  }
}

void timer_set_current_time(struct timer *t) {
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  t->time = (uint64_t) ((1000000000 * (int64_t)time.tv_sec) + (int64_t)time.tv_nsec);
}

void timer_bang(struct timer *t, counter_t* counter) {
  // fire the callback
  // FIXME: actually should post an event with cb pointer, idx?
  // alternatively: just use a single cb function and post (idx, count)
  t->cb(t->idx, *counter);
  *counter = *counter + 1;
  
#if BUSYWAIT // testing
  uint64_t target = t->time + t->delta;
  while( t->time < target) {
	timer_set_current_time(t);
  }
#else
  // with nanosleep(), thread can be preempted while sleeping.
  // for accuracy, tell system clock to wake us at absolute time:
  struct timespec ts;
  t->time += t->delta;
  ts.tv_sec = (time_t) t->time / 1000000000;
  ts.tv_nsec = (long) t->time % 1000000000;
  clock_nanosleep (CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
#endif
}

void timer_wait(int idx) {
  pthread_join(timers[idx].tid, NULL);
}

#undef MAX_NUM_TIMERS_OK
