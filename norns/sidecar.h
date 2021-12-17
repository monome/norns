
#ifndef _NORNS_SIDECAR_H_
#define _NORNS_SIDECAR_H_

#include <stdlib.h>

// main loop of sidecar server process
// (FIXME: never exits! at all!)
int sidecar_server_main();

// intialize sidecar client IPC connections
int sidecar_client_init();

// request a command to be executed by the sidecar server
// blocks until completion and response
// result is a null-terminated string stored in `*result`
void sidecar_client_cmd(char **result, size_t *size, const char *cmd);

#endif