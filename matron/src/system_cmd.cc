#include <stdio.h>
#include <string.h>

#include "events.h"
#include "sidecar.h"

//----------------------------
//--- types and variables

static void post_command_capture(const char *cmd, void *ctx, const char *response_buff, size_t response_size) {
    int cb_ref = (int)ctx;

    if (response_size == 0) {
        fprintf(stderr, "system_cmd: command (%s) failed\n", cmd);
        return;
    }

    // response buffer is only valid during the callback so it is copied so that
    // it is valid for the lifetime of the event
    char *capture = (char *)malloc(response_size);
    memcpy(capture, response_buff, response_size);

    union event_data *ev = event_data_new(EVENT_SYSTEM_CMD);
    // this will get freed when the event is handled
    ev->system_cmd.capture = capture;
    ev->system_cmd.cb_ref = cb_ref;
    event_post(ev);
}

//-------------------------------
//-- extern function definitions

bool system_cmd(const char *cmd, int cb_ref) {
    return sidecar_client_cmd_async(cmd, (void *)cb_ref, post_command_capture);
}