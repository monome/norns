// tests for matron/src/clocks/clock_internal.c behavior
// covers thread loop, tempo publish, restarts

#include <chrono>
#include <cmath>
#include <doctest/doctest.h>
#include <thread>

extern "C" {
#include "clocks/clock_internal.h"
#include "helpers.h"
}

extern "C" {
void clock_internal_test_enable_threadless(bool enable);
void clock_internal_test_tick_once();
void clock_internal_test_set_ticks(unsigned long long v);
unsigned long long clock_internal_test_get_published_ticks(void);
void clock_internal_test_reset_published_ticks(void);
void clock_internal_test_stop_thread(void);
double clock_internal_test_get_last_sleep_s(void);
double clock_internal_test_get_last_next_tick_time(void);
double clock_internal_test_get_last_current_time(void);
double clock_internal_test_get_last_tick_duration(void);
}

TEST_CASE("internal clock") {
    // most tests run in threadless mode.
    // init runs once.
    tests_set_now(0.0);
    clock_internal_test_enable_threadless(true);
    clock_internal_init();

    SUBCASE("default tempo is 120 bpm after init") {
        CHECK(clock_internal_get_tempo() == doctest::Approx(120.0));
    }

    SUBCASE("set_tempo updates on next tick (not immediately)") {
        // initial 120 bpm
        REQUIRE(clock_internal_get_tempo() == doctest::Approx(120.0));

        // change tempo; without a tick, reported tempo remains the old value
        clock_internal_set_tempo(90.0);
        CHECK(clock_internal_get_tempo() == doctest::Approx(120.0));

        // after one tick publish, reported tempo reflects the new value
        clock_internal_test_tick_once();
        CHECK(clock_internal_get_tempo() == doctest::Approx(90.0));
    }

    SUBCASE("long-run tick stepping still publishes tempo changes") {
        // run 50000 ticks to verify the publish path keeps working
        for (int i = 0; i < 50000; ++i) {
            clock_internal_test_tick_once();
        }

        // change tempo and confirm it publishes on the next tick
        clock_internal_set_tempo(60.0);
        CHECK(clock_internal_get_tempo() != doctest::Approx(60.0)); // not yet published
        clock_internal_test_tick_once();
        CHECK(clock_internal_get_tempo() == doctest::Approx(60.0));
    }

    SUBCASE("tick counter does not overflow near INT_MAX") {
        // seed counter near 32-bit signed max to simulate long uptime
        // INT_MAX - 2 = 2147483645; step a few times across the boundary
        unsigned long long near_int_max = 2147483645ULL;
        clock_internal_test_set_ticks(near_int_max);

        // capture beat and ensure monotonic increase across several ticks
        double prev_beat = clock_internal_get_beat();
        for (int i = 0; i < 10; ++i) {
            clock_internal_test_tick_once();
            double beat = clock_internal_get_beat();
            CHECK(beat > prev_beat);
            prev_beat = beat;
        }

        // tempo should remain finite and at default 120 bpm (no tempo change applied)
        CHECK(clock_internal_get_tempo() == doctest::Approx(120.0));
    }

    SUBCASE("manual tick publishes tempo changes and survives time jump") {
        // initial tempo is 120
        REQUIRE(clock_internal_get_tempo() == doctest::Approx(120.0));

        // change tempo; until a tick publishes, reported tempo remains previous value
        clock_internal_set_tempo(90.0);
        CHECK(clock_internal_get_tempo() == doctest::Approx(120.0));
        clock_internal_test_tick_once();
        CHECK(clock_internal_get_tempo() == doctest::Approx(90.0));

        // simulate a large forward jump in system time—publish still works on the next tick
        tests_set_now(6.0 * 3600.0);
        clock_internal_set_tempo(60.0);
        CHECK(clock_internal_get_tempo() == doctest::Approx(90.0));
        clock_internal_test_tick_once();
        CHECK(clock_internal_get_tempo() == doctest::Approx(60.0));
    }

    SUBCASE("restart resets beat and applies latest tempo on next publish") {
        // advance a few ticks so beat > 0
        for (int i = 0; i < 10; ++i) {
            clock_internal_test_tick_once();
        }
        REQUIRE(clock_internal_get_beat() > 0.0);

        // change tempo and restart; next publish should reset beat to 0 and apply tempo
        clock_internal_set_tempo(75.0);
        clock_internal_restart();
        clock_internal_test_tick_once();
        CHECK(clock_internal_get_beat() == doctest::Approx(0.0));
        CHECK(clock_internal_get_tempo() == doctest::Approx(75.0));
    }

    // --- threaded tests ---
    // these tests run the real background thread and must be managed carefully
    // to avoid leaking threads or interfering with other tests.

    SUBCASE("loop rebases after long time jump") {
        // simulate a JACK stall—nanosleep does not advance time; enable strict validation for invalid sleeps
        tests_set_now(0.0);
        tests_set_nanosleep_advances_time(false);
        tests_set_nanosleep_strict_validation(true);

        // enable threading for this subcase only
        clock_internal_test_enable_threadless(false);
        clock_internal_test_reset_published_ticks();
        clock_internal_init();

        // wait for at least one publish to initialize pacing
        unsigned long long p0 = clock_internal_test_get_published_ticks();
        for (int i = 0; i < 2000000 && clock_internal_test_get_published_ticks() == p0; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // jump time forward by hours so next_tick_time is far behind current_time
        tests_set_now(6.0 * 3600.0);

        // the rebase logic should skip catchup ticks and quickly produce non-negative sleep
        bool became_non_negative = false;
        for (int i = 0; i < 50; ++i) {
            // wait for next publish
            unsigned long long before = clock_internal_test_get_published_ticks();
            for (int s = 0; s < 100000 && clock_internal_test_get_published_ticks() == before; ++s) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            double last_sleep = clock_internal_test_get_last_sleep_s();
            if (last_sleep >= 0.0) {
                became_non_negative = true;
                break;
            }
        }
        CHECK(became_non_negative == true);

        // cleanup: stop the thread and restore nanosleep behavior
        clock_internal_test_stop_thread();
        tests_set_nanosleep_advances_time(true);
        tests_set_nanosleep_strict_validation(false);
        clock_internal_test_enable_threadless(true);
    }

    SUBCASE("threaded loop survives long time jump and publishes tempo changes") {
        // run the real clock thread; nanosleep is stubbed to advance virtual time
        tests_set_now(0.0);
        clock_internal_test_reset_published_ticks();
        clock_internal_test_enable_threadless(false);
        clock_internal_init();

        // wait for the first publish
        unsigned long long pub = clock_internal_test_get_published_ticks();
        for (int i = 0; i < 2000000 && clock_internal_test_get_published_ticks() == pub; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        REQUIRE(clock_internal_get_tempo() == doctest::Approx(120.0));

        // change tempo to 90 and wait for it to be observed.
        clock_internal_set_tempo(90.0);
        pub = clock_internal_test_get_published_ticks();
        for (int i = 0; i < 2000000; ++i) {
            if (std::abs(clock_internal_get_tempo() - 90.0) < 0.01)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        CHECK(clock_internal_get_tempo() == doctest::Approx(90.0));

        // jump time forward by hours; tempo change still publishes on the next tick
        tests_set_now(6.0 * 3600.0);
        clock_internal_set_tempo(60.0);
        for (int i = 0; i < 4000000; ++i) {
            if (std::abs(clock_internal_get_tempo() - 60.0) < 0.01)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        CHECK(clock_internal_get_tempo() == doctest::Approx(60.0));

        // stop the background thread to avoid affecting other tests
        clock_internal_test_stop_thread();
        clock_internal_test_enable_threadless(true);
    }
}
