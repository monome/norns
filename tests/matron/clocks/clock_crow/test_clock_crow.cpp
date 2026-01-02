// tests for matron/src/clocks/clock_crow.c behavior
// covers tempo measurement and reschedule logic

#include <cmath>
#include <doctest/doctest.h>

extern "C" {
#include "clocks/clock_crow.h"
#include "helpers.h"
}

static int g_reschedule_calls = 0;

// override the helpers.c stub to track reschedule calls for this test
extern "C" void clock_reschedule_sync_events_from_source(clock_source_t source) {
    if (source == CLOCK_SOURCE_CROW) {
        g_reschedule_calls++;
    }
}

// warmup iterations to fill duration buffer and reach steady state (size 4 + 1)
static const int BUFFER_WARMUP_ITERATIONS = 5;

TEST_CASE("crow: init sets default tempo 120") {
    tests_set_now(100.0);
    clock_crow_init();
    CHECK(clock_crow_get_tempo() == doctest::Approx(120.0));
}

TEST_CASE("crow: in_div change triggers reschedule on next clock") {
    g_reschedule_calls = 0;
    tests_set_now(10.0);
    clock_crow_init();

    // first tick sets last time
    clock_crow_handle_clock();

    // change divider and tick again with non-trivial dt
    clock_crow_in_div(4);
    tests_set_now(10.0 + 0.02);
    clock_crow_handle_clock();

    CHECK(g_reschedule_calls == 1);
}

TEST_CASE("crow: tempo follows measured interval and divider ratio") {
    double time = 0.0;
    tests_set_now(time);
    clock_crow_init();
    clock_crow_handle_clock(); // set last time

    clock_crow_in_div(4);
    for (int i = 0; i < BUFFER_WARMUP_ITERATIONS; ++i) {
        time += 0.125;
        tests_set_now(time);
        clock_crow_handle_clock();
    }
    double tempo1 = clock_crow_get_tempo();
    CHECK(tempo1 > 0.0);

    clock_crow_in_div(2);
    for (int i = 0; i < BUFFER_WARMUP_ITERATIONS; ++i) {
        time += 0.125;
        tests_set_now(time);
        clock_crow_handle_clock();
    }
    double tempo2 = clock_crow_get_tempo();
    CHECK(tempo2 > tempo1);
    // expect approx 2x increase when divider halves (tolerate smoothing error)
    CHECK(tempo2 / tempo1 == doctest::Approx(2.0).epsilon(0.25));
}

TEST_CASE("crow: tempo remains finite with very high frequency ticks") {
    double time = 0.0;
    tests_set_now(time);
    clock_crow_init();
    clock_crow_handle_clock();

    clock_crow_in_div(4);
    double dt = 0.0001; // 0.1ms between ticks, simulates thousands of BPM
    for (int i = 0; i < BUFFER_WARMUP_ITERATIONS; ++i) {
        time += dt;
        tests_set_now(time);
        clock_crow_handle_clock();
    }

    double tempo = clock_crow_get_tempo();
    // verify no inf or NaN (smoothing may produce unusual values with extreme timing)
    CHECK(std::isfinite(tempo));
    CHECK(!std::isnan(tempo));
}

TEST_CASE("crow: tempo remains finite with very low frequency ticks") {
    double time = 0.0;
    tests_set_now(time);
    clock_crow_init();
    clock_crow_handle_clock();

    clock_crow_in_div(4);
    // 0.9s between ticks -> 3.6s beat_duration (just under 4s threshold)
    double dt = 0.9;
    for (int i = 0; i < BUFFER_WARMUP_ITERATIONS; ++i) {
        time += dt;
        tests_set_now(time);
        clock_crow_handle_clock();
    }

    double tempo = clock_crow_get_tempo();
    // tempo should be very low and finite
    CHECK(tempo > 0.0);
    CHECK(tempo < 20.0); // sanity check: should be very slow tempo
    CHECK(std::isfinite(tempo));
    CHECK(!std::isnan(tempo));
}

TEST_CASE("crow: zero dt ticks do not produce inf or NaN") {
    double time = 0.0;
    tests_set_now(time);
    clock_crow_init();
    clock_crow_handle_clock();

    clock_crow_in_div(4);
    // first tick with normal dt
    time += 0.125;
    tests_set_now(time);
    clock_crow_handle_clock();

    // send tick with exact same timestamp (dt = 0)
    clock_crow_handle_clock();

    double tempo = clock_crow_get_tempo();
    // verify no inf or NaN despite zero dt
    CHECK(std::isfinite(tempo));
    CHECK(!std::isnan(tempo));
}

TEST_CASE("crow: ticks beyond 4s beat_duration threshold reset timing") {
    double time = 0.0;
    tests_set_now(time);
    clock_crow_init();
    clock_crow_handle_clock();

    clock_crow_in_div(4);
    double dt = 0.25; // 60 BPM equivalent
    for (int i = 0; i < BUFFER_WARMUP_ITERATIONS; ++i) {
        time += dt;
        tests_set_now(time);
        clock_crow_handle_clock();
    }
    double tempo_before = clock_crow_get_tempo();
    CHECK(std::isfinite(tempo_before));

    // send tick beyond 4s beat_duration threshold (dt=5.0, div=4 -> beat_duration=20.0)
    // this triggers the "clock stopped" reset path which updates last_time but not buffer
    time += 5.0;
    tests_set_now(time);
    clock_crow_handle_clock();

    // tempo should be unchanged (buffer wasn't updated)
    double tempo_after = clock_crow_get_tempo();
    CHECK(tempo_after == doctest::Approx(tempo_before));
    CHECK(std::isfinite(tempo_after));
    CHECK(!std::isnan(tempo_after));
}
