/*
 * osc.c
 *
 * user OSC device, send/receive arbitrary OSC within lua scripts
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "lo/lo.h"

#include "args.h"
#include "events.h"
#include "oracle.h"

static lo_address remote_addr;
//static lo_server_thread st;

void osc_init(void) { 
    // TEST ADDRESS/PORT
    remote_addr = lo_address_new("192.168.1.6", "9001"); 
}

void osc_deinit(void) {
    lo_address_free(remote_addr);
}

int osc_send(const char *path, lo_message msg) {
    lo_send(remote_addr, path, msg);
    free(msg);
    return 0;
} 
