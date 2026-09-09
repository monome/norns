// tests for norns/sidecar.cpp

#include <doctest/doctest.h>
#include <stdlib.h>
#include <string.h>

#include "sidecar.h"

// helpers for the tests below.
namespace {

void *failing_calloc(size_t, size_t) {
    return nullptr;
}

int strdup_successes_left = 0;
char *counting_strdup(const char *s) {
    if (strdup_successes_left <= 0) {
        return nullptr;
    }
    strdup_successes_left--;
    return strdup(s);
}

void ignore_chunk(void *, const char *, size_t) {
}
void ignore_done(const char *, void *, const sidecar_status_t *) {
}

// restores the real allocators when a test ends
struct seam_guard {
    ~seam_guard() {
        sidecar_test_calloc = calloc;
        sidecar_test_strdup = strdup;
    }
};

} // namespace

TEST_CASE("sidecar_client_cmd_async: reports failure when the request cannot be allocated") {
    seam_guard guard;
    sidecar_test_calloc = failing_calloc;

    CHECK(sidecar_client_cmd_async("echo hi", 0, nullptr, ignore_chunk, ignore_done) == false);
}

TEST_CASE("sidecar_client_cmd_async: reports failure when the command cannot be copied") {
    seam_guard guard;
    strdup_successes_left = 0;
    sidecar_test_strdup = counting_strdup;

    CHECK(sidecar_client_cmd_async("echo hi", 0, nullptr, ignore_chunk, ignore_done) == false);
}

TEST_CASE("sidecar_client_cmd_async: enqueues and reports success when allocation works") {
    CHECK(sidecar_client_cmd_async("echo hi", 0, nullptr, ignore_chunk, ignore_done) == true);
}

TEST_CASE("sidecar_client_detach_async: reports failure when either copy fails") {
    seam_guard guard;
    sidecar_test_strdup = counting_strdup;

    strdup_successes_left = 0;
    CHECK(sidecar_client_detach_async("cmd", "unit", nullptr, ignore_done) == false);

    strdup_successes_left = 1;
    CHECK(sidecar_client_detach_async("cmd", "unit", nullptr, ignore_done) == false);
}
