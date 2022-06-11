#include <assert.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

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
//--- request queue

typedef enum {
    REQUEST_FIRST_REQUEST = 0,
    REQUEST_RUN_CMD,
} request_type;

struct request_common {
    request_type type;
};

struct request_run_cmd {
    struct request_common common;
    char *cmd;
    void *ctx;
    client_cmd_completion_t completion_handler;
};

union request {
    request_type type;
    struct request_run_cmd run_cmd;
};

struct req_node {
    struct req_node *next;
    struct req_node *prreq;
    union request *req;
};

struct req_q {
    struct req_node *head;
    struct req_node *tail;
    ssize_t size;
    pthread_cond_t nonempty;
    pthread_mutex_t lock;
};

static struct req_q requests;

static void requests_push(union request *req) {
    struct req_node *node = (struct req_node*)calloc(1, sizeof(struct req_node));
    node->req = req;
    if (requests.size == 0) {
        insque(node, NULL);
        requests.head = node;
    } else {
        insque(node, requests.tail);
    }
    requests.tail = node;
    requests.size += 1;
}

static union request *requests_pop() {
    struct req_node *node = requests.head;
    if (node == NULL) {
        return NULL;
    }
    union request *req = node->req;
    requests.head = node->next;
    if (node == requests.tail) {
        assert(requests.size == 1);
        requests.tail = NULL;
    }
    remque(node);
    free(node);
    requests.size -= 1;
    return req;
}

static void requests_init(void) {
    requests.size = 0;
    requests.head = NULL;
    requests.tail = NULL;
    pthread_cond_init(&requests.nonempty, NULL);
}

static union request *request_new(request_type type) {
    // FIXME: better not to allocate here, use object pool
    union request *req = (request*)calloc(1, sizeof(union request));
    req->type = type;
    return req;
}

static void request_post(union request *req) {
    assert(req != NULL);
    pthread_mutex_lock(&requests.lock);
    if (requests.size == 0) {
        // signal handler thread to wake up...
        pthread_cond_signal(&requests.nonempty);
    }
    requests_push(req);
    // ...handler actually wakes up once we release the lock
    pthread_mutex_unlock(&requests.lock);

}


//---------------------------------
//--- server

const size_t MAX_CAPTURE_BYTES = 1024 * 1000;  // 10MB maximum
const size_t READ_BUFFER_BYTES = 8192;
static char read_buffer[READ_BUFFER_BYTES];

static nng_msg *sidecar_server_run_cmd(const char *cmd) {
  FILE *f = popen((char *)cmd, "r");
  if (f == NULL) {
    fprintf(stderr, "popen() failed\n");
    return NULL;
  }

  int status;
  nng_msg *msg = NULL;
  size_t capture_bytes = 0;
  size_t read_bytes = 0;

  // NOTE: initial allocation is 0 because nng_msg_append* place values at the
  // end of the msg, allocating more space as needed. allocating space up front
  // does not behave like a capacity reservation.
  status = nng_msg_alloc(&msg, 0);
  if (status != 0) {
    fprintf(stderr, "nng_msg_alloc() failed\n");
    return NULL;
  }

  while ((read_bytes = fread(read_buffer, 1, READ_BUFFER_BYTES, f)) > 0) {
    status = nng_msg_append(msg, read_buffer, read_bytes);
    if (status != 0) {
      pclose(f);
      nng_msg_free(msg);
      fprintf(stderr, "nng_msg_append() failed\n");
      return NULL;
    }

    capture_bytes += read_bytes;

    if (capture_bytes > MAX_CAPTURE_BYTES) {
      pclose(f);
      nng_msg_free(msg);
      fprintf(stderr, "sidecar: command output too large, %d bytes maxiumum\n", MAX_CAPTURE_BYTES);
      return NULL;
    }
  }

  // append a null byte to the end of the buffer
  nng_msg_append_u16(msg, 0);

  pclose(f);
  return msg;
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
    size_t cmd_sz = 0;

    if ((rv = nng_recv(sock, &cmd, &cmd_sz, NNG_FLAG_ALLOC)) != 0) {
      sidecar_nng_error("recv error", rv);
      continue;
    }

    nng_msg *response = sidecar_server_run_cmd(cmd);
    if (response != NULL) {
      if ((rv = nng_sendmsg(sock, response, 0)) != 0) {
        nng_msg_free(response);
        sidecar_nng_error("send error", rv);
      }
    }

    if (cmd != NULL) {
      nng_free(cmd, cmd_sz);
    }
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

static pthread_t client_thread;
static struct client_state cs;
static pthread_mutex_t run_cmd_lock;

static void handle_run_cmd(const char *cmd, void *ctx, client_cmd_completion_t completion) {
  static char empty_result[1] = { '\0' };

  int rv;
  char *result = NULL;
  size_t result_sz = 0;
  char *recv_buf = NULL;
  size_t recv_sz = 0;

  // serialize interaction with the sidecar so sync and async command invocation
  // is not interlieved
  pthread_mutex_lock(&run_cmd_lock);

  if ((rv = nng_send(cs.sock, (void *)cmd, strlen(cmd) + 1, 0)) != 0) {
    sidecar_nng_error("sidecar: send error", rv);
    goto complete;
  }

  // TODO: investigate using a pre-allocated receive buffer
  if ((rv = nng_recv(cs.sock, &recv_buf, &recv_sz, NNG_FLAG_ALLOC)) != 0) {
    sidecar_nng_error("sidecar: recv error", rv);
    goto complete;
  }

  if (recv_sz > 0) {
    result = recv_buf;
    result_sz = recv_sz;
  } else {
    result = empty_result;
    result_sz = 1;
  }

complete:
  pthread_mutex_unlock(&run_cmd_lock);
  completion(cmd, ctx, result, result_sz);
  nng_free(recv_buf, recv_sz);
}

static void handle_request(union request *req) {
  switch (req->type) {
  case REQUEST_RUN_CMD:
    handle_run_cmd(req->run_cmd.cmd, req->run_cmd.ctx, req->run_cmd.completion_handler);
    free(req->run_cmd.cmd);
    break;
  default:
    fprintf(stderr, "sidecar: unhandled event type; %d\n", req->type);
    break;
  }
}

static void *sidecar_client_loop(void *) {
  union request *req = NULL;

  for (;;) {
    pthread_mutex_lock(&requests.lock);
    while (requests.size == 0) {
      pthread_cond_wait(&requests.nonempty, &requests.lock);
    }
    req = requests_pop();
    pthread_mutex_unlock(&requests.lock);
    if (req != NULL) {
      handle_request(req);
    }
  }

  return NULL;
}

//
// external
//

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

  requests_init();

  if (pthread_create(&client_thread, NULL, sidecar_client_loop, NULL)) {
    fprintf(stderr, "sidecar: unable to create client thread\n");
    return -1;
  }
  pthread_setname_np(client_thread, "client_loop");
  pthread_detach(client_thread);

  return 0;
}

void sidecar_client_cleanup() {
  // TODO: terminate the request loop and join the thread
}

bool sidecar_client_cmd_async(const char *cmd, void *ctx, client_cmd_completion_t cb) {
  union request *req = request_new(REQUEST_RUN_CMD);
  req->run_cmd.cmd = strdup(cmd);
  req->run_cmd.ctx = ctx;
  req->run_cmd.completion_handler = cb;
  request_post(req);
  return true;
}

bool sidecar_client_cmd(const char *cmd, void *ctx, client_cmd_completion_t cb) {
  handle_run_cmd(cmd, ctx, cb);
  return true;
}

typedef struct {
  char **result;
  size_t *result_size;
} buffer_fill_ctx;

static void _buffer_fill_completion(const char *cmd, void *ctx, const char *buf, size_t size) {
  buffer_fill_ctx *output = (buffer_fill_ctx *)ctx;
  *(output->result_size) = size;
  if (size != 0) {
    *(output->result) = (char *)malloc(size);
    memcpy(*(output->result), buf, size);
  } else {
    *(output->result) = NULL;
  }
}

void sidecar_client_cmd(const char *cmd, char **dst, size_t *dst_size) {
  buffer_fill_ctx ctx = { dst, dst_size };
  handle_run_cmd(cmd, &ctx, _buffer_fill_completion);
}
