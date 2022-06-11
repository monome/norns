
#ifndef _NORNS_SIDECAR_H_
#define _NORNS_SIDECAR_H_

#include <stdlib.h>

typedef void (*client_cmd_completion_t)(const char *cmd, void *ctx, const char *buff, size_t size);

// main loop of sidecar server process
// (FIXME: never exits! at all!)
int sidecar_server_main();

// intialize sidecar client IPC connections
int sidecar_client_init();
void sidecar_client_cleanup();

// request a command to be executed asynchronously by the sidecar server
// returns immediately after enqueing command
// calls completion function from a background thread with the command output
bool sidecar_client_cmd_async(const char *cmd, void *ctx, client_cmd_completion_t cb);

// request a command be executed synchronously by the sidecar server
// calls completion function for the invoking thread
bool sidecar_client_cmd(const char *cmd, void *ctx, client_cmd_completion_t cb);

// request a command be executed synchronously by the sidecar server and returns
// the command output in result and size. if the command did not return any
// output size will be 0 and result will be NULL.
//
// the caller is responsible for free()ing result
void sidecar_client_cmd(const char *cmd, char **result, size_t *size);

#endif