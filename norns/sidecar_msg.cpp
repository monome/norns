#include <string.h>

#include "sidecar_msg.h"

enum {
    END_HOW_EXITED = 0,
    END_HOW_SIGNALLED = 1,
};

static void put_u32(uint8_t *out, uint32_t v) {
    out[0] = (uint8_t)(v & 0xFF);
    out[1] = (uint8_t)((v >> 8) & 0xFF);
    out[2] = (uint8_t)((v >> 16) & 0xFF);
    out[3] = (uint8_t)((v >> 24) & 0xFF);
}

static uint32_t get_u32(const uint8_t *in) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
}

static size_t encode_payload(uint8_t *out, size_t cap, sidecar_msg_type_t type, uint8_t req_id, const char *payload, size_t len) {
    const size_t frame = SIDECAR_MSG_HEADER_BYTES + len + 1;
    if (out != NULL && cap >= frame) {
        out[0] = (uint8_t)type;
        out[1] = req_id;
        if (len > 0) {
            memcpy(out + SIDECAR_MSG_HEADER_BYTES, payload, len);
        }
        out[frame - 1] = 0;
    }
    return frame;
}

size_t sidecar_msg_encode_chunk(uint8_t *out, size_t cap, uint8_t req_id, const char *payload, size_t len) {
    return encode_payload(out, cap, SIDECAR_MSG_CHUNK, req_id, payload, len);
}

size_t sidecar_msg_encode_end(uint8_t *out, size_t cap, uint8_t req_id, bool signalled, uint8_t code) {
    const size_t frame = SIDECAR_MSG_HEADER_BYTES + SIDECAR_MSG_STATUS_BYTES;
    if (out != NULL && cap >= frame) {
        out[0] = (uint8_t)SIDECAR_MSG_END;
        out[1] = req_id;
        out[SIDECAR_MSG_HEADER_BYTES] = signalled ? END_HOW_SIGNALLED : END_HOW_EXITED;
        out[SIDECAR_MSG_HEADER_BYTES + 1] = code;
    }
    return frame;
}

size_t sidecar_msg_encode_error(uint8_t *out, size_t cap, uint8_t req_id, const char *text) {
    if (text == NULL) {
        text = "";
    }
    return encode_payload(out, cap, SIDECAR_MSG_ERROR, req_id, text, strlen(text));
}

size_t sidecar_msg_encode_cmd(uint8_t *out, size_t cap, uint8_t req_id, uint32_t timeout_ms, const char *cmd) {
    if (cmd == NULL) {
        cmd = "";
    }
    const size_t cmd_len = strlen(cmd);
    const size_t frame = SIDECAR_MSG_HEADER_BYTES + SIDECAR_MSG_TIMEOUT_BYTES + cmd_len + 1;
    if (out != NULL && cap >= frame) {
        out[0] = (uint8_t)SIDECAR_MSG_CMD;
        out[1] = req_id;
        put_u32(out + SIDECAR_MSG_HEADER_BYTES, timeout_ms);
        memcpy(out + SIDECAR_MSG_HEADER_BYTES + SIDECAR_MSG_TIMEOUT_BYTES, cmd, cmd_len + 1);
    }
    return frame;
}

size_t sidecar_msg_encode_detach(uint8_t *out, size_t cap, uint8_t req_id, const char *unit, const char *cmd) {
    if (unit == NULL) {
        unit = "";
    }
    if (cmd == NULL) {
        cmd = "";
    }
    const size_t unit_len = strlen(unit);
    const size_t cmd_len = strlen(cmd);
    const size_t frame = SIDECAR_MSG_HEADER_BYTES + unit_len + 1 + cmd_len + 1;
    if (out != NULL && cap >= frame) {
        out[0] = (uint8_t)SIDECAR_MSG_DETACH;
        out[1] = req_id;
        memcpy(out + SIDECAR_MSG_HEADER_BYTES, unit, unit_len + 1);
        memcpy(out + SIDECAR_MSG_HEADER_BYTES + unit_len + 1, cmd, cmd_len + 1);
    }
    return frame;
}

static bool type_is_known(uint8_t tag) {
    switch (tag) {
    case SIDECAR_MSG_CHUNK:
    case SIDECAR_MSG_END:
    case SIDECAR_MSG_ERROR:
    case SIDECAR_MSG_CMD:
    case SIDECAR_MSG_DETACH:
        return true;
    default:
        return false;
    }
}

bool sidecar_msg_parse(const void *body, size_t len, sidecar_frame_t *out) {
    memset(out, 0, sizeof(*out));

    if (body == NULL || len < SIDECAR_MSG_HEADER_BYTES) {
        return false;
    }

    const char *bytes = (const char *)body;
    if (!type_is_known((uint8_t)bytes[0])) {
        return false;
    }
    const sidecar_msg_type_t type = (sidecar_msg_type_t)(uint8_t)bytes[0];
    const uint8_t req_id = (uint8_t)bytes[1];

    const char *payload = bytes + SIDECAR_MSG_HEADER_BYTES;
    const size_t region = len - SIDECAR_MSG_HEADER_BYTES;

    const bool terminated = region > 0 && payload[region - 1] == '\0';

    switch (type) {
    case SIDECAR_MSG_END: {
        if (region != SIDECAR_MSG_STATUS_BYTES) {
            return false;
        }
        const uint8_t how = (uint8_t)payload[0];
        if (how != END_HOW_EXITED && how != END_HOW_SIGNALLED) {
            return false;
        }
        out->signalled = (how == END_HOW_SIGNALLED);
        out->code = (uint8_t)payload[1];
        break;
    }

    case SIDECAR_MSG_CHUNK:
    case SIDECAR_MSG_ERROR:
        if (!terminated) {
            return false;
        }
        out->payload = payload;
        out->payload_len = region - 1;
        break;

    case SIDECAR_MSG_CMD: {
        if (region < SIDECAR_MSG_TIMEOUT_BYTES + 1 || !terminated) {
            return false;
        }
        out->timeout_ms = get_u32((const uint8_t *)payload);
        out->cmd = payload + SIDECAR_MSG_TIMEOUT_BYTES;
        break;
    }

    case SIDECAR_MSG_DETACH: {
        if (!terminated) {
            return false;
        }
        const size_t unit_len = strnlen(payload, region);
        if (unit_len + 1 >= region) {
            return false;
        }
        out->unit = payload;
        out->cmd = payload + unit_len + 1;
        break;
    }
    }

    out->type = type;
    out->req_id = req_id;
    return true;
}
