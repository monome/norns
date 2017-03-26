#include <assert.h>
#include <search.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "events.h"
#include "m.h"
#include "oracle.h"
#include "weaver.h"

#include "event_types.h"

//----------------------------
//--- types and variables

struct ev_node {
  struct ev_node* next;
  struct ev_node* prev;
  union event_data* ev;
};

struct ev_q {
  struct ev_node* head;
  struct ev_node* tail;
  ssize_t size;
  pthread_cond_t nonempty;
  pthread_mutex_t lock;
};

struct ev_q evq;
bool quit;

//----------------------------
//--- static function declarations

//---- handlers
static inline void handle_event(union event_data* ev);
static inline void handle_exec_code_line(struct event_exec_code_line* ev);
static inline void handle_timer(struct event_timer* ev);
static inline void handle_monome_add(struct event_monome_add* ev);
static inline void handle_monome_remove(struct event_monome_remove* ev);
static inline void handle_grid_key(struct event_grid_key* ev);
static inline void handle_input_add(struct event_input_add* ev);
static inline void handle_input_remove(struct event_input_remove* ev);
static inline void handle_input_event(struct event_input_event* ev);
static inline void handle_engine_report(void);
static inline void handle_command_report(void);
static inline void handle_quit(void);

// call with the queue locked
static inline void ev_q_add(union event_data* ev) {
  struct ev_node *evn = calloc(1, sizeof(struct ev_node));
  assert(ev != NULL);
  evn->ev = ev;
  insque(evn, evq.tail);
  evq.tail = evn;
  if(evq.size == 0) {
	evq.head = evn;
  }
  evq.size++;
}

// call with the queue locked
static inline void evq_rem(struct ev_node* evn) {
  if(evq.head == evn) { evq.head = NULL; }
  if(evq.tail == evn) { evq.tail = evn->prev; }
  remque(evn);
  evq.size--;
  // free the event and node memory here
  // FIXME: theoretically faster to use an object pool
  free(evn->ev);
  free(evn);
}

//-------------------------------
//-- extern function definitions

void events_init(void) {
  evq.size = 0;
  evq.head = NULL;
  evq.tail = NULL;
  pthread_cond_init(&evq.nonempty, NULL);
}

union event_data* event_data_new(event_t type) {
  // FIXME: theoretically faster to use an object pool
  union event_data* ev = calloc(1, sizeof(union event_data));
  ev->type = type;
  return ev;
}

// add an event to the q and signal if necessary
void event_post(union event_data * ev) {
  assert(ev != NULL);
  pthread_mutex_lock(&evq.lock);
  if(evq.size == 0) {
	// signal handler thread to wake up... 
	pthread_cond_signal(&evq.nonempty);
  }
  ev_q_add(ev);
  // ...handler actually wakes up once we release the lock
  pthread_mutex_unlock(&evq.lock);
}

// main loop to read events!
void event_loop(void) {
  union event_data ev;
  struct ev_node *evn;
  while(!quit) {
	pthread_mutex_lock(&evq.lock);
	// while() because contention may produce spurious wakeup
	while(evq.size == 0) {
	  // atomically unlocks the mutex, sleeps on condvar, locks again on wakeup
	  pthread_cond_wait(&evq.nonempty, &evq.lock);
	}
	assert(evq.size > 0);
	assert(evq.tail != NULL);
	evn = evq.tail;
	memcpy(&ev, evn->ev, sizeof(union event_data));
	evq_rem(evn); // frees event memory and decrements q size
	pthread_mutex_unlock(&evq.lock);
	handle_event(&ev);
  }
}

//------------------------------
//-- static function definitions

static void handle_event(union event_data* ev) {
  switch(ev->type) {
  case EVENT_EXEC_CODE_LINE:
	handle_exec_code_line(&(ev->exec_code_line));
	break;
  case EVENT_TIMER:
	handle_timer(&(ev->timer));
	break;
  case EVENT_MONOME_ADD:
	handle_monome_add(&(ev->monome_add));
	break;
  case EVENT_MONOME_REMOVE:
	handle_monome_remove(&(ev->monome_remove));
	break;
  case EVENT_GRID_KEY:
	handle_grid_key(&(ev->grid_key));
	break;
  case EVENT_INPUT_ADD:
	handle_input_add(&(ev->input_add));
	break;
  case EVENT_INPUT_REMOVE:
	handle_input_remove(&(ev->input_remove));
	break;
  case EVENT_INPUT_EVENT:
	handle_input_event(&(ev->input_event));
	break;
  case EVENT_ENGINE_REPORT:
	handle_engine_report();
	break;
  case EVENT_COMMAND_REPORT:
	handle_command_report();
	break;
  case  EVENT_QUIT:
	handle_quit();
	break;
  }
}

//---------------------------------
//---- handlers

//--- code execution
void handle_exec_code_line(struct event_exec_code_line *ev) {
  w_handle_line(ev->line);
  free(ev->line);
}

//--- timers
void handle_timer(struct event_timer *ev) {
  w_handle_timer(ev->id, ev->stage);
}

//--- monome devices
void handle_monome_add(struct event_monome_add *ev) {
  w_handle_monome_add(ev->dev);
}

void handle_monome_remove(struct event_monome_remove *ev) {
  w_handle_monome_remove(ev->id);
}

void handle_grid_key(struct event_grid_key *ev) {
  w_handle_grid_key(ev->id, ev->x, ev->y, ev->state);
}

//--- input devices
void handle_input_add(struct event_input_add *ev) {
  w_handle_input_add(ev->dev);
}

void handle_input_remove(struct event_input_remove *ev) {
  w_handle_input_remove(ev->id);
}

void handle_input_event(struct event_input_event *ev) {
  w_handle_input_event(ev->id, ev->type, ev->code, ev->value);
}

//--- TODO: MIDI

//--- reports
void handle_engine_report(void) {
  o_lock_descriptors();
  const char** p = o_get_engine_names();
  const int n = o_get_num_engines();
  w_handle_engine_report(p, n);
  o_unlock_descriptors();
}

void handle_command_report(void) {
  o_lock_descriptors();
  const struct engine_command* p = o_get_commands();
  const int n = o_get_num_commands();
  w_handle_command_report(p, n);
  o_unlock_descriptors();
}

//--- quit
void handle_quit(void) {
  quit = true;
}
