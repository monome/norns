/* 
   oracle.c

   implements communication with audio server for C programs.

   user should not care about the method (IPC or otherwise.)

   for now, we will use OSC with liblo.

*/

  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "lo/lo.h"

#include "args.h"
#include "events.h"
#include "oracle.h"

static lo_address remote_addr;
static lo_server_thread st;

//-------------------
//--- audio engine descriptor management


// count of audio engine descriptors
int num_engines = 0;
// count of parameter descriptors
int num_params = 0;
// count of audio buffer descriptors
int num_buffers = 0;

// max count of any single desciptor type
#define MAX_NUM_DESC 1024

char *engine_names[MAX_NUM_DESC];
char *param_names[MAX_NUM_DESC];
char *buffer_names[MAX_NUM_DESC];

// mutex for desctiptor data
pthread_mutex_t desc_lock;

//---------------------------------
//--- static functions

// FIXME: pretty bad function design here,
// better to use descriptor type enum or something,
// but these are just static utils and probably temporary.

// intialize descriptor lists
static void o_init_descriptors(void);
// clear a given array of descriptors
static void o_clear_desc(char** desc_arr, int num);
// set a given entry in a given descriptor list
static void o_set_desc(char** desc_arr, int idx, const char* name);
// set a given descriptor count variable
static void o_set_num_desc(int* dst, int num);

//--- OSC handlers
static int buffer_report_start(const char *path, const char *types,
							   lo_arg ** argv, int argc,
							   void *data, void *user_data);
static int buffer_report_name(const char *path, const char *types,
							  lo_arg ** argv, int argc,
							  void *data, void *user_data);
static int buffer_report_end(const char *path, const char *types,
							 lo_arg ** argv, int argc,
							 void *data, void *user_data);
static int engine_report_start(const char *path, const char *types,
							   lo_arg ** argv, int argc,
							   void *data, void *user_data);
static int engine_report_name(const char *path, const char *types,
							  lo_arg ** argv, int argc,
							  void *data, void *user_data);
static int engine_report_end(const char *path, const char *types,
							 lo_arg ** argv, int argc,
							 void *data, void *user_data) ;
static int param_report_start(const char *path, const char *types,
							  lo_arg ** argv, int argc,
							  void *data, void *user_data);
static int param_report_name(const char *path, const char *types,
							 lo_arg ** argv, int argc,
							 void *data, void *user_data);
static int param_report_end(const char *path, const char *types,
							lo_arg ** argv, int argc,
							void *data, void *user_data);
static void lo_error_handler(int num, const char *m, const char *path);

//-----------------------------------
//---- extern function definitions

//--- init
void o_init(void) {
  const char *loc_port = args_local_port();
  const char *rem_port = args_remote_port();
  printf("starting OSC server: listening on port %s, sending to port %s\n",
		 loc_port, rem_port);
  o_init_descriptors();
  
  remote_addr = lo_address_new(NULL, rem_port);
  st = lo_server_thread_new(loc_port, lo_error_handler);

  //  buffer report sequence
  lo_server_thread_add_method(st, "/buffer/report/start", "i",
							  buffer_report_start, NULL);
  lo_server_thread_add_method(st, "/buffer/report/name", "is",
							  buffer_report_name, NULL);
  lo_server_thread_add_method(st, "/buffer/report/end", "",
							  buffer_report_end, NULL);
 
  // engine report sequence
  lo_server_thread_add_method(st, "/engine/report/start", "i",
							  engine_report_start, NULL);
  lo_server_thread_add_method(st, "/engine/report/name", "is",
							  engine_report_name, NULL);
  lo_server_thread_add_method(st, "/engine/report/end", "",
							  engine_report_end, NULL);

  // param report sequence
  lo_server_thread_add_method(st, "/param/report/start", "i",
							  param_report_start, NULL);
  lo_server_thread_add_method(st, "/param/report/name", "is",
							  param_report_name, NULL);
  lo_server_thread_add_method(st, "/param/report/end", "",
							  param_report_end, NULL);

  lo_server_thread_start(st);
}

void o_deinit(void) {
  printf("killing audio engine\n");
  lo_send(remote_addr, "/engine/kill", "");
  printf("stopping OSC server\n");
  lo_server_thread_free(st);
}


//--- descriptor access
int o_get_num_buffers(void) {
  return num_buffers;
}
int o_get_num_engines(void) {
  return num_engines;
}
int o_get_num_params(void) {
  return num_params;
}
const char** o_get_buffer_names(void) {
  return (const char**)buffer_names;
}
const char** o_get_engine_names(void) {
  return (const char**)engine_names;
}
const char** o_get_param_names(void) {
  return (const char**)param_names;
}

//-- mutex access
void o_lock_descriptors() {
  //  printf("o_lock_descriptors() \n");
  int res = pthread_mutex_lock(&desc_lock);
  if(res) {
	printf("o_lock_descriptors failed with code %d \b", res); 
  }  
}

void o_unlock_descriptors() {
  //  printf("o_unlock_descriptors() \n");
  int res = pthread_mutex_unlock(&desc_lock);
  if(res)  {
	printf("o_unlock_descriptors failed with code %d \b", res); 
  }  
}

//--- tranmission to audio engine

void o_request_engine_report(void) {
  lo_send(remote_addr, "/engine/request/report", "");
}

void o_load_engine(const char* name) {
  lo_send(remote_addr, "/engine/load/name", "s", name);
}


//   FIXME: autogenerate from protcol description?
//   use dynamic list of OSC patterns (varargs tricks)?
void o_load_buffer_name(const char* name, const char* path) {
  lo_send(remote_addr, "/buffer/load/name", "ss", name, path);
}

void o_set_param_name(const char* name, const float val) {
  lo_send(remote_addr, "/param/set/name", "sf", name, val);
}

void o_set_param_index(int idx, const float val) {
  lo_send(remote_addr, "/param/set/idx", "if", idx, val);
}


//-------------------------
//--- static function definitions
void o_init_descriptors(void) {
  pthread_mutex_init(&desc_lock, NULL);
  for(int i=0; i<MAX_NUM_DESC; i++) {
	buffer_names[i] = NULL;
	engine_names[i] = NULL;
	param_names[i] = NULL;
  }
}

// clear a given array of descriptors
void o_clear_desc(char** desc_arr, int num) {
  o_lock_descriptors();
  for(int i=0; i<num; i++) {
	if(desc_arr[i] != NULL) {
	  free(desc_arr[i]);
	  desc_arr[i] = NULL;
	} else {
	  printf("o_clear_desc: encountered unexpected null entry \n"); 
	}
  }
  o_unlock_descriptors();
}

// set a given entry in a given descriptor list
void o_set_desc(char** desc_arr, int idx, const char* name) {
  size_t len;
  o_lock_descriptors();
  if(desc_arr[idx] != NULL) {
	printf("refusing to allocate descriptor %d; already exists", idx);
  } else {
	len = strlen(name);
	desc_arr[idx] = malloc(len);
	if ( desc_arr[idx] == NULL ) {
	  printf("failure to malloc for descriptor %d : %s \n", idx, name);
	} else {
	  strncpy(desc_arr[idx], name, len+1);
	}
  }  
  o_unlock_descriptors();
}

// set a given descriptor count variable
void o_set_num_desc(int* dst, int num) { 
  o_lock_descriptors();
  *dst = num;
  o_unlock_descriptors();
}

//---- OSC handlers


int buffer_report_start(const char *path, const char *types, lo_arg ** argv,
						int argc, void *data, void *user_data) {
  o_clear_desc(buffer_names, num_buffers);
  // single arg is count of buffers
  o_set_num_desc(&num_buffers, argv[0]->i);
}

int buffer_report_name(const char *path, const char *types, lo_arg ** argv,
					   int argc, void *data, void *user_data) {
  // arg 1: buffer index
  // arg 2: buffer name
  o_set_desc(buffer_names, argv[0]->i, &argv[1]->s);
}

int buffer_report_end(const char *path, const char *types, lo_arg ** argv,
					  int argc, void *data, void *user_data) {
  // no arguments
  event_post(EVENT_BUFFER_REPORT, NULL, NULL);
}

int engine_report_start(const char *path, const char *types,
						lo_arg ** argv, int argc, void *data, void *user_data)
{
  o_clear_desc(engine_names, num_engines);
  // single arg is count
  o_set_num_desc(&num_engines, argv[0]->i);
}

int engine_report_name(const char *path, const char *types, lo_arg ** argv,
					   int argc, void *data, void *user_data) {
  // arg 1: buffer index
  // arg 2: buffer name
  // NB: yes, this is the correct way to read a string from a lo_arg
  o_set_desc(engine_names, argv[0]->i, &argv[1]->s);
}

int engine_report_end(const char *path, const char *types, lo_arg ** argv,
					  int argc, void *data, void *user_data) {
  // no arguments; post event
  // FIXME: as yet no outstanding need for report_end message to occur at all.
  // could add counter from report_start to double-check the param count.
  // or (best) we could simply use binary blobs from Crone,
  // replacing the whole response sequence with a single message
  // (downside: nasty blob-construction code in supercollider)
  event_post(EVENT_ENGINE_REPORT, NULL, NULL);
}

int param_report_start(const char *path, const char *types, lo_arg ** argv,
					   int argc, void *data, void *user_data)
{
  o_clear_desc(param_names, num_params);
  // single arg is count of params
  o_set_num_desc(&num_params, argv[0]->i);
}

int param_report_name(const char *path, const char *types, lo_arg ** argv,
					  int argc, void *data, void *user_data) {
  // arg 1: buffer index
  // arg 2: buffer name
  // NB: yes, this is the correct way to read a string from a lo_arg
  o_set_desc(param_names, argv[0]->i, &argv[1]->s);
}

int param_report_end(const char *path, const char *types, lo_arg ** argv,
					 int argc, void *data, void *user_data) {
  // no arguments; post event
  event_post(EVENT_PARAM_REPORT, NULL, NULL);
}

void lo_error_handler(int num, const char *m, const char *path) {
  printf("liblo error %d in path %s: %s\n", num, path, m);
  fflush(stdout);
}
