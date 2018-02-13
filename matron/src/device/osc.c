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
static lo_server_thread st;

static int osc_receive(const char *path, const char *types,
			lo_arg **argv, int argc, void *data, void *user_data);
static void lo_error_handler(int num, const char *m, const char *path);

void osc_init(void) { 
    // arbitrary default destination ip/port
    remote_addr = lo_address_new("127.0.0.1", "9001"); 

    // receive
    st = lo_server_thread_new("10111", lo_error_handler); 
    lo_server_thread_add_method(st, NULL, NULL, osc_receive, NULL); 
    lo_server_thread_start(st);
}

void osc_deinit(void) {
    lo_address_free(remote_addr);
    lo_server_thread_free(st);
}

void osc_send(const char *path, lo_message msg) {
    lo_send_message(remote_addr, path, msg);
    free(msg);
} 

void osc_remote_addr(const char *ip, const char *port) {
    free(remote_addr);
    remote_addr = lo_address_new(ip,port);
}

int osc_receive(const char *path,
                       const char *types,
                       lo_arg **argv,
                       int argc,
                       void *data,
                       void *user_data)
{
    (void)path;
    (void)types;
    (void)argc;
    (void)argv;
    (void)data;
    (void)user_data;

	int i;
    printf("path: <%s>\n", path);
    for (i = 0; i < argc; i++) {
        printf("arg %d '%c' ", i, types[i]);
        lo_arg_pp((lo_type)types[i], argv[i]);
        printf("\n");
    }
    printf("\n");
    fflush(stdout);

    return 0;
}

void lo_error_handler(int num, const char *m, const char *path) {
    printf("liblo error %d in path %s: %s\n", num, path, m);
    fflush(stdout);
}
