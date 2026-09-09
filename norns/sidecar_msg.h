#ifndef _NORNS_SIDECAR_MSG_H_
#define _NORNS_SIDECAR_MSG_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// sidecar wire protocol. every frame is a type tag, a request id echoed on
// each reply, and a payload.
//
//   norns -> sidecar
//     CMD    [0x10][req id][timeout ms][command][0]
//     DETACH [0x11][req id][unit name][0][command][0]
//   sidecar -> norns
//     CHUNK  [0x01][req id][raw output bytes, may contain null characters][0]
//     END    [0x02][req id][how][code]
//     ERROR  [0x03][req id][error text][0]
//
// END means the command ran to completion and carries how it ended, ERROR means
// it did not, so a non-zero exit is an END and a command that never started is
// an ERROR.

#define SIDECAR_MSG_HEADER_BYTES 2
#define SIDECAR_MSG_TIMEOUT_BYTES 4
#define SIDECAR_MSG_STATUS_BYTES 2

typedef enum {
    // sidecar -> norns
    SIDECAR_MSG_CHUNK = 0x01,
    SIDECAR_MSG_END = 0x02,
    SIDECAR_MSG_ERROR = 0x03,
    // norns -> sidecar
    SIDECAR_MSG_CMD = 0x10,
    SIDECAR_MSG_DETACH = 0x11,
} sidecar_msg_type_t;

typedef struct {
    sidecar_msg_type_t type;
    uint8_t req_id;
    const char *payload;
    size_t payload_len;
    const char *unit;
    const char *cmd;
    uint32_t timeout_ms; // CMD only, longest quiet stretch between output, zero for no ceiling
    bool signalled;      // END only, true when code is a signal
    uint8_t code;        // END only
} sidecar_frame_t;

size_t sidecar_msg_encode_chunk(uint8_t *out, size_t cap, uint8_t req_id, const char *payload, size_t len);
size_t sidecar_msg_encode_end(uint8_t *out, size_t cap, uint8_t req_id, bool signalled, uint8_t code);
size_t sidecar_msg_encode_error(uint8_t *out, size_t cap, uint8_t req_id, const char *text);
size_t sidecar_msg_encode_cmd(uint8_t *out, size_t cap, uint8_t req_id, uint32_t timeout_ms, const char *cmd);
size_t sidecar_msg_encode_detach(uint8_t *out, size_t cap, uint8_t req_id, const char *unit, const char *cmd);
bool sidecar_msg_parse(const void *body, size_t len, sidecar_frame_t *out);

#endif // _NORNS_SIDECAR_MSG_H_
