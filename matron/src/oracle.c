/* 
oracle.c

implements communication with audio server for C programs.

user should not care about the method (IPC or otherwise.)

for now, we will use OSC. 

shared memory might be preferable in the future.

*/

  
#include <stdio.h>
#include "lo/lo.h"

#include "oracle.h"

static const int remote_port = 57120;
static lo_address remote_addr;

void o_init(void) {
  char str[8];
  snprintf(str, 8, "%d", remote_port);
  remote_addr = lo_address_new(NULL, str);
}

//-----------------------
//--- IPC functions

//   FIXME: autogenerate from protcol description?
//   use dynamic list of OSC patterns?

void o_load_module(const char* name) {
   lo_send(remote_addr, "/module/load", "s", name);
}

void o_load_buffer(const char* name, const char* path) {
  lo_send(remote_addr, "/buffer/load", "ss", name, path);
}

void o_set_param_name(const char* name, const float val) {
  lo_send(remote_addr, "/param/set", "sf", name, val);
}

void o_set_param_index(int idx, const float val) {
  lo_send(remote_addr, "/param/set", "if", idx, val);
}
