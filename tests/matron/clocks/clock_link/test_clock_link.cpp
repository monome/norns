// tests for matron/src/clocks/clock_link.c behavior
// covers abl_link integration and helpers

#include <doctest/doctest.h>

extern "C" {
#include "abl_link.h"
#include "clocks/clock_link.h"
#include "helpers.h"
}

// declare test-only seam (not in public header)
extern "C" void clock_link_run_once(void);

// override helpers.c stubs to return fixed values for link tests
extern "C" double clock_get_reference_beat(clock_reference_t *) {
    return 42.0;
}
extern "C" double clock_get_reference_tempo(clock_reference_t *) {
    return 96.0;
}

// access stub globals from abl_link_stubs.c
extern "C" volatile uint64_t g_stub_num_peers;
extern "C" volatile int g_stub_created_count;
extern "C" volatile double g_stub_last_quantum;
extern "C" volatile double g_stub_requested_tempo;
extern "C" volatile double g_stub_beat;
extern "C" volatile double g_stub_tempo;
extern "C" volatile int64_t g_stub_micros;
extern "C" volatile bool g_stub_is_playing;
extern "C" volatile bool g_stub_enabled;
extern "C" volatile bool g_stub_sync_enabled;
extern "C" volatile int g_stub_set_tempo_call_count;

// capture start/stop requests from clock_link when start/stop sync reacts
static int g_link_start_calls = 0;
static int g_link_stop_calls = 0;

extern "C" void clock_start_from_source(clock_source_t source) {
    if (source == CLOCK_SOURCE_LINK) {
        g_link_start_calls++;
    }
}
extern "C" void clock_stop_from_source(clock_source_t source) {
    if (source == CLOCK_SOURCE_LINK) {
        g_link_stop_calls++;
    }
}

TEST_CASE("link: init constructs abl_link instance") {
    g_stub_created_count = 0;
    clock_link_init();
    CHECK(g_stub_created_count == 1);
}

TEST_CASE("link: number_of_peers proxies to abl_link") {
    g_stub_num_peers = 7;
    CHECK(clock_link_number_of_peers() == 7u);
}

TEST_CASE("link: getters call reference helpers") {
    clock_link_init();
    CHECK(clock_link_get_beat() == doctest::Approx(42.0));
    CHECK(clock_link_get_tempo() == doctest::Approx(96.0));
}

TEST_CASE("link: clock_link_run_once seam steps one iteration with updated quantum") {
    // initialize without starting background thread.
    clock_link_init();

    // set stub state and change quantum.
    // single iteration should pass it through.
    g_stub_last_quantum = -1.0;
    g_stub_beat = 1.0;
    g_stub_tempo = 120.0;
    g_stub_micros = 0;
    clock_link_set_quantum(8.0);

    clock_link_run_once();
    CHECK(g_stub_last_quantum == doctest::Approx(8.0));
}

TEST_CASE("link: set_tempo publishes requested tempo to stub on next iteration") {
    clock_link_init();

    g_stub_requested_tempo = 0.0;
    g_stub_micros = 0;
    g_stub_tempo = 120.0;
    g_stub_beat = 0.0;

    clock_link_set_tempo(135.0);
    CHECK(g_stub_requested_tempo == doctest::Approx(0.0));

    // value is applied when the loop runs once
    clock_link_run_once();
    CHECK(g_stub_requested_tempo == doctest::Approx(135.0));
}

TEST_CASE("link: start/stop sync triggers clock start/stop from LINK source") {
    clock_link_init();

    // reset counters and stub state
    g_link_start_calls = 0;
    g_link_stop_calls = 0;
    g_stub_is_playing = false; // link initially not playing

    // enable sync, transition from not playing -> playing -> not playing
    clock_link_set_start_stop_sync(true);

    // Start: link begins playing
    g_stub_is_playing = true;
    clock_link_run_once();
    CHECK(g_link_start_calls == 1);
    CHECK(g_link_stop_calls == 0);

    // Stop: link stops playing
    g_stub_is_playing = false;
    clock_link_run_once();
    CHECK(g_link_start_calls == 1);
    CHECK(g_link_stop_calls == 1);
}

TEST_CASE("link: join/leave session toggles abl_link enable state") {
    clock_link_init();
    // ensure known baseline
    g_stub_enabled = false;

    // join should enable link on next step
    clock_link_join_session();
    clock_link_run_once();
    CHECK(g_stub_enabled == true);

    // leave should disable link on next step
    clock_link_leave_session();
    clock_link_run_once();
    CHECK(g_stub_enabled == false);
}

TEST_CASE("link: start/stop sync flag forwarded to abl_link") {
    clock_link_init();
    g_stub_sync_enabled = false;

    clock_link_set_start_stop_sync(true);
    clock_link_run_once();
    CHECK(g_stub_sync_enabled == true);

    clock_link_set_start_stop_sync(false);
    clock_link_run_once();
    CHECK(g_stub_sync_enabled == false);
}

TEST_CASE("link: transport start/stop directly set playing state") {
    clock_link_init();
    g_stub_is_playing = false;

    clock_link_set_transport_start();
    clock_link_run_once();
    CHECK(g_stub_is_playing == true);

    clock_link_set_transport_stop();
    clock_link_run_once();
    CHECK(g_stub_is_playing == false);
}

TEST_CASE("link: set_tempo applies once and clears request") {
    clock_link_init();
    g_stub_set_tempo_call_count = 0;

    clock_link_set_tempo(111.0);
    // first step applies
    clock_link_run_once();
    // second step should not reapply
    clock_link_run_once();
    CHECK(g_stub_set_tempo_call_count == 1);
}
