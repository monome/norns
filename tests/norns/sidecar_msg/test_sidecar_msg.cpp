// tests for norns/sidecar_msg.cpp, the frame encoder and parser.

#include <cstring>
#include <doctest/doctest.h>
#include <vector>

#include "sidecar_msg.h"

// per-frame encode helpers for the tests below.
namespace {

constexpr size_t WIRE_HEADER_BYTES = 2;  // type tag + req id
constexpr size_t WIRE_TIMEOUT_BYTES = 4; // CMD only
constexpr size_t WIRE_STATUS_BYTES = 2;  // END only

std::vector<uint8_t> encode_chunk(uint8_t req_id, const char *payload, size_t len) {
    size_t n = sidecar_msg_encode_chunk(nullptr, 0, req_id, payload, len);
    std::vector<uint8_t> buf(n);
    sidecar_msg_encode_chunk(buf.data(), buf.size(), req_id, payload, len);
    return buf;
}

std::vector<uint8_t> encode_error(uint8_t req_id, const char *text) {
    size_t n = sidecar_msg_encode_error(nullptr, 0, req_id, text);
    std::vector<uint8_t> buf(n);
    sidecar_msg_encode_error(buf.data(), buf.size(), req_id, text);
    return buf;
}

std::vector<uint8_t> encode_cmd(uint8_t req_id, uint32_t timeout_ms, const char *cmd) {
    size_t n = sidecar_msg_encode_cmd(nullptr, 0, req_id, timeout_ms, cmd);
    std::vector<uint8_t> buf(n);
    sidecar_msg_encode_cmd(buf.data(), buf.size(), req_id, timeout_ms, cmd);
    return buf;
}

std::vector<uint8_t> encode_end(uint8_t req_id, bool signalled, uint8_t code) {
    size_t n = sidecar_msg_encode_end(nullptr, 0, req_id, signalled, code);
    std::vector<uint8_t> buf(n);
    sidecar_msg_encode_end(buf.data(), buf.size(), req_id, signalled, code);
    return buf;
}

std::vector<uint8_t> encode_detach(uint8_t req_id, const char *unit, const char *cmd) {
    size_t n = sidecar_msg_encode_detach(nullptr, 0, req_id, unit, cmd);
    std::vector<uint8_t> buf(n);
    sidecar_msg_encode_detach(buf.data(), buf.size(), req_id, unit, cmd);
    return buf;
}

} // namespace

//---------------------------------
//--- round trips

TEST_CASE("sidecar_msg: a CMD frame round-trips") {
    auto buf = encode_cmd(7, 240000, "ls -la");

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.type == SIDECAR_MSG_CMD);
    CHECK(f.req_id == 7);
    CHECK(f.timeout_ms == 240000);
    REQUIRE(f.cmd != nullptr);
    CHECK(std::strcmp(f.cmd, "ls -la") == 0);
}

TEST_CASE("sidecar_msg: a CMD timeout of zero means no ceiling and round-trips as zero") {
    auto buf = encode_cmd(7, 0, "sleep 300");

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.timeout_ms == 0);
    CHECK(std::strcmp(f.cmd, "sleep 300") == 0);
}

TEST_CASE("sidecar_msg: a CMD timeout uses the full width of its field") {
    auto buf = encode_cmd(1, UINT32_MAX, "x");

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.timeout_ms == UINT32_MAX);
}

TEST_CASE("sidecar_msg: a DETACH frame round-trips both of its strings") {
    auto buf = encode_detach(9, "norns-update", "/bin/bash update.sh");

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.type == SIDECAR_MSG_DETACH);
    CHECK(f.req_id == 9);
    REQUIRE(f.unit != nullptr);
    REQUIRE(f.cmd != nullptr);
    CHECK(std::strcmp(f.unit, "norns-update") == 0);
    CHECK(std::strcmp(f.cmd, "/bin/bash update.sh") == 0);
}

TEST_CASE("sidecar_msg: an END frame carries how the command ended") {
    auto buf = encode_end(3, false, 0);

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.type == SIDECAR_MSG_END);
    CHECK(f.req_id == 3);
    CHECK_FALSE(f.signalled);
    CHECK(f.code == 0);
    CHECK(f.payload == nullptr);
    CHECK(f.payload_len == 0);
}

TEST_CASE("sidecar_msg: an END frame round-trips a non-zero exit code") {
    auto buf = encode_end(3, false, 7);

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK_FALSE(f.signalled);
    CHECK(f.code == 7);
}

TEST_CASE("sidecar_msg: an END frame tells a signal apart from an exit") {
    auto buf = encode_end(3, true, 9);

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.signalled);
    CHECK(f.code == 9);
}

TEST_CASE("sidecar_msg: an END frame carries the highest exit code a shell can report") {
    auto buf = encode_end(3, false, 255);

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.code == 255);
}

TEST_CASE("sidecar_msg: a CHUNK frame round-trips, and its length excludes the terminator") {
    auto buf = encode_chunk(1, "hello\n", 6);

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.type == SIDECAR_MSG_CHUNK);
    CHECK(f.req_id == 1);
    REQUIRE(f.payload != nullptr);
    CHECK(f.payload_len == 6);
    CHECK(std::memcmp(f.payload, "hello\n", 6) == 0);
    CHECK(f.payload[f.payload_len] == '\0');
}

TEST_CASE("sidecar_msg: an ERROR frame round-trips as a readable C string") {
    auto buf = encode_error(2, "exit status 1");

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.type == SIDECAR_MSG_ERROR);
    CHECK(f.req_id == 2);
    REQUIRE(f.payload != nullptr);
    CHECK(f.payload_len == 13);
    CHECK(std::strcmp(f.payload, "exit status 1") == 0);
}

//---------------------------------
//--- payload edge cases

TEST_CASE("sidecar_msg: a zero-length CHUNK is legal") {
    auto buf = encode_chunk(4, "", 0);
    CHECK(buf.size() == WIRE_HEADER_BYTES + 1); // just the terminator

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.type == SIDECAR_MSG_CHUNK);
    CHECK(f.payload_len == 0);
    REQUIRE(f.payload != nullptr);
    CHECK(f.payload[0] == '\0');
}

TEST_CASE("sidecar_msg: an empty ERROR is legal") {
    auto buf = encode_error(5, "");

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.type == SIDECAR_MSG_ERROR);
    CHECK(f.payload_len == 0);
}

TEST_CASE("sidecar_msg: a CHUNK carrying null characters round-trips byte for byte") {
    // chunk length is carried in the frame, not derived from strlen, so
    // interior null characters survive
    const char raw[] = {'a', '\0', 'b', '\0', 'c'};
    auto buf = encode_chunk(6, raw, sizeof(raw));

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.payload_len == sizeof(raw));
    CHECK(std::memcmp(f.payload, raw, sizeof(raw)) == 0);
}

TEST_CASE("sidecar_msg: an empty command and an empty unit are still terminated") {
    auto buf = encode_detach(8, "", "");

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(buf.data(), buf.size(), &f));
    CHECK(f.unit[0] == '\0');
    CHECK(f.cmd[0] == '\0');
}

//---------------------------------
//--- sizing

TEST_CASE("sidecar_msg: sizing with a null buffer agrees with the filled length") {
    CHECK(sidecar_msg_encode_cmd(nullptr, 0, 1, 0, "abc") == WIRE_HEADER_BYTES + WIRE_TIMEOUT_BYTES + 3 + 1);
    CHECK(sidecar_msg_encode_error(nullptr, 0, 1, "ab") == WIRE_HEADER_BYTES + 2 + 1);
    CHECK(sidecar_msg_encode_chunk(nullptr, 0, 1, "abcd", 4) == WIRE_HEADER_BYTES + 4 + 1);
    CHECK(sidecar_msg_encode_end(nullptr, 0, 1, false, 0) == WIRE_HEADER_BYTES + WIRE_STATUS_BYTES);
    CHECK(sidecar_msg_encode_detach(nullptr, 0, 1, "u", "c") == WIRE_HEADER_BYTES + (1 + 1) + (1 + 1));
}

TEST_CASE("sidecar_msg: encoding into a too-small output buffer writes nothing but still returns the size") {
    uint8_t buf[4], buf_before[4];
    std::memset(buf, 0xAA, sizeof(buf));
    std::memset(buf_before, 0xAA, sizeof(buf_before));

    CHECK(sidecar_msg_encode_cmd(buf, sizeof(buf), 1, 0, "abc") == WIRE_HEADER_BYTES + WIRE_TIMEOUT_BYTES + 3 + 1);
    CHECK(std::memcmp(buf, buf_before, sizeof(buf)) == 0);

    CHECK(sidecar_msg_encode_end(buf, 1, 1, false, 0) == WIRE_HEADER_BYTES + WIRE_STATUS_BYTES);
    CHECK(std::memcmp(buf, buf_before, sizeof(buf)) == 0);
}

//---------------------------------
//--- malformed frames

TEST_CASE("sidecar_msg: a frame shorter than a header is rejected") {
    const uint8_t one[] = {SIDECAR_MSG_END};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(one, 1, &f));
    CHECK_FALSE(sidecar_msg_parse(one, 0, &f));
    CHECK_FALSE(sidecar_msg_parse(nullptr, 0, &f));
}

TEST_CASE("sidecar_msg: an unknown type byte is rejected") {
    sidecar_frame_t f;
    for (unsigned tag = 0; tag <= 0xFF; ++tag) {
        if (tag == SIDECAR_MSG_CHUNK || tag == SIDECAR_MSG_END || tag == SIDECAR_MSG_ERROR ||
            tag == SIDECAR_MSG_CMD || tag == SIDECAR_MSG_DETACH) {
            continue;
        }
        const uint8_t frame[] = {(uint8_t)tag, 1, 'x', 0};
        CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
    }
}

TEST_CASE("sidecar_msg: a CMD whose command never terminates is rejected") {
    const uint8_t frame[] = {SIDECAR_MSG_CMD, 1, 0, 0, 0, 0, 'l', 's'};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
}

TEST_CASE("sidecar_msg: a CMD too short to hold its timeout is rejected") {
    const uint8_t frame[] = {SIDECAR_MSG_CMD, 1, 0, 0, 0};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
}

TEST_CASE("sidecar_msg: a CMD carrying a timeout but no command is rejected") {
    const uint8_t frame[] = {SIDECAR_MSG_CMD, 1, 0, 0, 0, 0};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
}

TEST_CASE("sidecar_msg: a CMD at its shortest legal size is accepted") {
    const uint8_t frame[] = {SIDECAR_MSG_CMD, 1, 0, 0, 0, 0, 0};

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(frame, sizeof(frame), &f));
    CHECK(f.timeout_ms == 0);
    CHECK(f.cmd[0] == '\0');
}

TEST_CASE("sidecar_msg: a CHUNK whose payload never terminates is rejected") {
    const uint8_t frame[] = {SIDECAR_MSG_CHUNK, 1, 'a', 'b'};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
}

TEST_CASE("sidecar_msg: an ERROR whose text never terminates is rejected") {
    const uint8_t frame[] = {SIDECAR_MSG_ERROR, 1, 'e'};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
}

TEST_CASE("sidecar_msg: a CMD with no payload at all is rejected") {
    const uint8_t frame[] = {SIDECAR_MSG_CMD, 1};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
}

TEST_CASE("sidecar_msg: an END missing its status is rejected") {
    const uint8_t bare[] = {SIDECAR_MSG_END, 1};
    const uint8_t half[] = {SIDECAR_MSG_END, 1, 0};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(bare, sizeof(bare), &f));
    CHECK_FALSE(sidecar_msg_parse(half, sizeof(half), &f));
}

TEST_CASE("sidecar_msg: an END carrying more than its status is rejected") {
    const uint8_t frame[] = {SIDECAR_MSG_END, 1, 0, 0, 0};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
}

TEST_CASE("sidecar_msg: an END whose how byte is neither exit nor signal is rejected") {
    const uint8_t frame[] = {SIDECAR_MSG_END, 1, 2, 0};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
}

TEST_CASE("sidecar_msg: a DETACH carrying only one string is rejected") {
    const uint8_t frame[] = {SIDECAR_MSG_DETACH, 1, 'u', 0};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
}

TEST_CASE("sidecar_msg: a DETACH whose command never terminates is rejected") {
    const uint8_t frame[] = {SIDECAR_MSG_DETACH, 1, 'u', 0, 'c'};

    sidecar_frame_t f;
    CHECK_FALSE(sidecar_msg_parse(frame, sizeof(frame), &f));
}

TEST_CASE("sidecar_msg: a DETACH at its shortest legal size is accepted") {
    const uint8_t frame[] = {SIDECAR_MSG_DETACH, 1, 0, 0};

    sidecar_frame_t f;
    REQUIRE(sidecar_msg_parse(frame, sizeof(frame), &f));
    CHECK(f.unit[0] == '\0');
    CHECK(f.cmd[0] == '\0');
}
