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

// TODO: semaphore for waiting on audio backend init
//static sem_t audio_init_sem;

//-------------------
//--- audio engine descriptor management

// count of audio engine descriptors
int num_engines = 0;
// count of command descriptors
int num_commands = 0;

// max count of any single desciptor type
#define MAX_NUM_DESC 1024

char *engine_names[MAX_NUM_DESC];
struct engine_command commands[MAX_NUM_DESC];

// mutex for desctiptor data
pthread_mutex_t desc_lock;

//---------------------------------
//--- static functions

// intialize descriptor lists
static void o_init_descriptors(void);
// clear a given array of descriptors
void o_clear_engine_names(void);

// set a given entry in the engine name list
static void o_set_engine_name(int idx, const char* name);
// set a given descriptor count variable
static void o_set_num_desc(int* dst, int num);

//--- OSC handlers

static int engine_report_start(const char *path, const char *types,
							   lo_arg ** argv, int argc,
							   void *data, void *user_data);
static int engine_report_entry(const char *path, const char *types,
							   lo_arg ** argv, int argc,
							   void *data, void *user_data);
static int engine_report_end(const char *path, const char *types,
							 lo_arg ** argv, int argc,
							 void *data, void *user_data) ;
static int command_report_start(const char *path, const char *types,
								lo_arg ** argv, int argc,
								void *data, void *user_data);
static int command_report_entry(const char *path, const char *types,
								lo_arg ** argv, int argc,
								void *data, void *user_data);
static int command_report_end(const char *path, const char *types,
							  lo_arg ** argv, int argc,
							  void *data, void *user_data);

static void lo_error_handler(int num, const char *m, const char *path);

// helper for va_list 
static int lo_send_varargs(const char *path, const char* types, va_list ap);
int lo_send_varargs(const char *path, const char* types, va_list ap) {
  int ret;
  lo_message msg = lo_message_new();
  
  /* remote_addr->errnum = 0; */
  /* remote_addr->errstring = 0; */
  
  //  ret = lo_message_add_varargs_internal(msg, types, ap, __FILE__, __LINE__);
}

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

  // engine report sequence
  lo_server_thread_add_method(st, "/report/engines/start", "i",
							  engine_report_start, NULL);
  lo_server_thread_add_method(st, "/report/engines/entry", "is",
							  engine_report_entry, NULL);
  lo_server_thread_add_method(st, "/report/engines/end", "",
							  engine_report_end, NULL);

  // command report sequence
  lo_server_thread_add_method(st, "/report/commands/start", "i",
							  command_report_start, NULL);
  lo_server_thread_add_method(st, "/report/commands/entry", "iss",
							  command_report_entry, NULL);
  lo_server_thread_add_method(st, "/report/commands/end", "",
							  command_report_end, NULL);

  lo_server_thread_start(st);
}

void o_deinit(void) {
  printf("killing audio engine\n");
  lo_send(remote_addr, "/engine/kill", "");
  printf("stopping OSC server\n");
  fflush(stdout);
  lo_server_thread_free(st);
}


//--- descriptor access
int o_get_num_engines(void) {
  return num_engines;
}

int o_get_num_commands(void) {
  return num_commands;
}

const char** o_get_engine_names(void) {
  return (const char**)engine_names;
}

const struct engine_command* o_get_commands(void) {
  return (const struct engine_command*)commands;
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
  printf("requesting engine report... \n");
  lo_send(remote_addr, "/report/engines", "");
}

void o_load_engine(const char* name) {
  printf("loading engine: %s \n", name);
  lo_send(remote_addr, "/engine/load/name", "s", name);
  /// trying this out...
  o_request_command_report();
}

void o_request_command_report(void) {
  lo_send(remote_addr, "/report/commands", "");
}

void o_send_command(const char* name, lo_message msg) {
  char* path;
  size_t len = sizeof(char) * (strlen(name) + 10);
  //  printf("o_send_command(); allocating %d bytes for path buffer\n", len);
  path = malloc(len);
  sprintf(path, "/command/%s", name);
  //  printf("path buffer: %s\n", path);
  lo_send_message(remote_addr, path, msg);
  free(msg);
}

//-------------------------
//--- static function definitions
void o_init_descriptors(void) {
  pthread_mutex_init(&desc_lock, NULL);
  for(int i=0; i<MAX_NUM_DESC; i++) {
	engine_names[i] = NULL;
	commands[i].cmd = NULL;
	commands[i].fmt = NULL;
  }
}

void o_clear_engine_names(void) {
  o_lock_descriptors();
  for(int i=0; i<num_engines; i++) {
	if(engine_names[i] != NULL) {
	  free(engine_names[i]);
	  engine_names[i] = NULL;
	} else {
	  printf("o_clear_engine_names: encountered unexpected null entry \n"); 
	}
  }
  o_unlock_descriptors();
}

void o_clear_commands(void) {
  o_lock_descriptors();
  for(int i=0; i<num_commands; i++) {
	if(commands[i].cmd != NULL && commands[i].fmt != NULL) {
	  free(commands[i].cmd);
	  free(commands[i].fmt);
	  commands[i].cmd = NULL;
	  commands[i].fmt = NULL;
	} else {
	  printf("o_clear_commands: encountered unexpected null entry \n");
	}
  }
  o_unlock_descriptors();
}


// set a given entry in engine name list
void o_set_engine_name(int idx, const char* name) {
  size_t len;
  o_lock_descriptors();
  if(engine_names[idx] != NULL) {
	printf("refusing to allocate engine name %d; already exists", idx);
  } else {
	len = strlen(name);
	engine_names[idx] = malloc(len);
	if ( engine_names[idx] == NULL ) {
	  printf("failure to malloc for engine name %d : %s \n", idx, name);
	} else {
	  strncpy(engine_names[idx], name, len+1);
	}
  }  
  o_unlock_descriptors();
}


// set a given entry in command list
void o_set_command(int idx, const char* cmd, const char* fmt) {
  size_t cmd_len, fmt_len;
  o_lock_descriptors();
  if(commands[idx].cmd != NULL || commands[idx].fmt != NULL) {
	printf("refusing to allocate command name %d; already exists", idx);
  } else {
	cmd_len = strlen(cmd);
	fmt_len = strlen(fmt);
	commands[idx].cmd = malloc(cmd_len +1);
	commands[idx].fmt = malloc(fmt_len +1);
	if ( commands[idx].cmd == NULL || commands[idx].fmt == NULL ) {
	  printf("failure to malloc for command %d : %s &s \n", idx, cmd, fmt);
	} else {
	  strncpy(commands[idx].cmd, cmd, cmd_len+1);
	  strncpy(commands[idx].fmt, fmt, fmt_len+1);
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
int engine_report_start(const char *path, const char *types,
						lo_arg ** argv, int argc, void *data, void *user_data)
{
  // arg 1: count of buffers
  //  printf("engine_report_start() : %s \n", path);
  o_clear_engine_names();
  o_set_num_desc(&num_engines, argv[0]->i);
}

int engine_report_entry(const char *path, const char *types, lo_arg ** argv,
						int argc, void *data, void *user_data) {
  // arg 1: buffer index
  // arg 2: buffer name
  
  //  printf("engine_report_entry() : %s \n", path);
  // NB: yes, this is the correct way to read a string from a lo_arg
  o_set_engine_name(argv[0]->i, &argv[1]->s);
}

int engine_report_end(const char *path, const char *types, lo_arg ** argv,
					  int argc, void *data, void *user_data) {
  // no arguments; post event
  //  printf("engine_report_end() : %s \n", path);
  // FIXME: as yet no outstanding need for report_end message to occur at all.
  // could add counter from report_start to double-check the param count.
  // or (better?) we could simply use binary blobs from Crone,
  // replacing the whole response sequence with a single message
  // (downside: nasty blob-construction code in supercollider)
  event_post(EVENT_ENGINE_REPORT, NULL, NULL);
}

//---------------------
//--- command report

int command_report_start(const char *path, const char *types, lo_arg ** argv,
						 int argc, void *data, void *user_data) {
  //  printf("command_report_start(): %d\n", argv[0]->i);
  o_clear_commands();
  o_set_num_desc(&num_commands, argv[0]->i);
}

int command_report_entry(const char *path, const char *types, lo_arg ** argv,
						 int argc, void *data, void *user_data) {
  
  //  printf("command_report_entry(): %d %s %s\n", argv[0]->i, &argv[1]->s, &argv[2]->s);
  o_set_command(argv[0]->i, &argv[1]->s, &argv[2]->s);
}

int command_report_end(const char *path, const char *types, lo_arg ** argv,
					   int argc, void *data, void *user_data) {
  //  printf("command_report_end()\n");
  event_post(EVENT_COMMAND_REPORT, NULL, NULL);
}



void lo_error_handler(int num, const char *m, const char *path) {
  printf("liblo error %d in path %s: %s\n", num, path, m);
  fflush(stdout);
}

