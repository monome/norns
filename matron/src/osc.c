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

#include <lo/lo.h>
#include <dns_sd.h>

#include "args.h"
#include "events.h"
#include "oracle.h"

static lo_address remote_addr;
static lo_server_thread st;
static DNSServiceRef dnssd_ref;

static int osc_receive(const char *path, const char *types,
			lo_arg **argv, int argc, lo_message msg, void *user_data);
static void lo_error_handler(int num, const char *m, const char *path);

void osc_init(void) {
    // arbitrary default destination ip/port
    remote_addr = lo_address_new("127.0.0.1", "9001");

    // receive
    st = lo_server_thread_new("10111", lo_error_handler);
    lo_server_thread_add_method(st, NULL, NULL, osc_receive, NULL);
    lo_server_thread_start(st);

    DNSServiceRegister(&dnssd_ref,
        0,
        0,
        "norns",
        "_osc._udp",
        NULL,
        NULL,
        htons(lo_server_thread_get_port(st)),
        0,
        NULL,
        NULL,
        NULL);
}

void osc_deinit(void) {
    DNSServiceRefDeallocate(dnssd_ref);
    lo_address_free(remote_addr);
    lo_server_thread_free(st);
}

void osc_send(const char *path, lo_message msg) {
    lo_send_message(remote_addr, path, msg);
    free(msg);
}

void osc_remote_addr(const char *ip, const char *port) {
    free(remote_addr);
    remote_addr = lo_address_new(ip, port);
}

int osc_receive(const char *path,
                       const char *types,
                       lo_arg **argv,
                       int argc,
                       lo_message msg,
                       void *user_data)
{
    (void)types;
    (void)argv;
    (void)argc;
    (void)user_data;

    union event_data *ev = event_data_new(EVENT_OSC);

    ev->osc_event.path = (char *) malloc(strlen(path) + 1);
    strcpy(ev->osc_event.path, path);

    ev->osc_event.msg = lo_message_clone(msg);
    event_post(ev);

    return 0;
}

void lo_error_handler(int num, const char *m, const char *path) {
    printf("liblo error %d in path %s: %s\n", num, path, m);
    fflush(stdout);
}
