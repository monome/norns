#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/transport/ipc/ipc.h>

#include "sidecar.h"

//---------------------------------
//--- common

const char *url = "ipc:///tmp/norns-sidecar.ipc";

static void sidecar_nng_error(const char *func, int rv) {
  fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
}

//---------------------------------
//--- server

const size_t CMD_CAPTURE_BYTES = 8192 * 8;

// allocates and returns a string buffer
static int sidecar_server_run_cmd(char **result, const char *cmd, size_t *sz) {
  FILE *f = popen((char *)cmd, "r");
  if (f == NULL) {
    fprintf(stderr, "popen() failed\n");
    return -1;
  }
  // FIXME: would be more efficient to allocate incrementally in chunks
  // instead of getting a huge blob up front and resizing down
  char *buf = (char *)malloc(CMD_CAPTURE_BYTES);
  buf[0] = '\0';
  size_t nb = fread(buf, 1, CMD_CAPTURE_BYTES - 1, f);
  buf[nb] = '\0';
  buf = (char *)realloc(buf, nb);
  *result = buf;
  *sz = nb;
  pclose(f);
  return 0;
}

int sidecar_server_main() {
  nng_socket sock;
  nng_listener listener;
  int rv;

  nng_ipc_register();

  if ((rv = nng_rep0_open(&sock)) != 0) {
    sidecar_nng_error("open socket failed", rv);
    return -1;
  }

  if ((rv = nng_listen(sock, url, &listener, 0)) != 0) {
    nng_close(sock);
    sidecar_nng_error("binding socket failed", rv);
    return -1;
  }

  for (;;) {
    char *cmd = NULL;
    char *result = NULL;
    size_t cmd_sz = 0;
    size_t result_sz = 0;

    if ((rv = nng_recv(sock, &cmd, &cmd_sz, NNG_FLAG_ALLOC)) != 0) {
      sidecar_nng_error("recv error", rv);
      continue;
    }

    sidecar_server_run_cmd(&result, cmd, &result_sz);

    if ((rv = nng_send(sock, result, result_sz, 0)) != 0) {
      sidecar_nng_error("send error", rv);
    }

    if (result) { free(result); }
    if (cmd) { nng_free(cmd, cmd_sz); }
  }

  return 0;
}

//---------------------------------
//--- client

struct client_state {
  nng_socket sock;
  nng_dialer dialer;
  bool initialized;
};

static struct client_state cs;

int sidecar_client_init() {
  int rv;

  nng_ipc_register();

  cs.initialized = false;

  if ((rv = nng_req0_open(&cs.sock)) != 0) {
    sidecar_nng_error("sidecar: open socket failed", rv);
    return -1;
  }

  // annoyingly this nng_dial is racing against the nng_listen in the sidecar
  // and unlike other transports the ipc transport appears to give up if the
  // dial is attempted before the listener is ready.
  int attempts = 5;
  while ( (attempts > 0) && ((rv = nng_dial(cs.sock, url, &cs.dialer, 0)) != 0) ) {
    sidecar_nng_error("sidecar: establishing connection failed", rv);
    attempts--;
    fprintf(stderr, "attempts left: %d\n", attempts);
    usleep(200 * 1000); // wait for 200 ms
  }

  if (rv == 0) {
    cs.initialized = true;
  }

  return 0;
}

void sidecar_client_cmd(char **result, size_t *size, const char *cmd) {
  int rv;
  char *recv_buf = NULL;
  size_t recv_sz = 0;

  if (!cs.initialized) {
    fprintf(stderr, "sidecar: client not initialized\n");
    return;
  }

  if ((rv = nng_send(cs.sock, (void *)cmd, strlen(cmd) + 1, 0)) != 0) {
    sidecar_nng_error("sidecar: send error", rv);
    return;
  }

  if ((rv = nng_recv(cs.sock, &recv_buf, &recv_sz, NNG_FLAG_ALLOC)) != 0) {
    sidecar_nng_error("sidecar: recv error", rv);
    return;
  }

  if (recv_sz > 0) {
    *result = (char *)malloc(recv_sz);
    *size = recv_sz;
    memcpy(*result, recv_buf, recv_sz);
  } else {
    *size = 1;
    *result = (char *)malloc(*size);
    *result[0] = '\0';
  }
}
