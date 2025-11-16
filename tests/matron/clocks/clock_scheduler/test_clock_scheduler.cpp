// tests for matron/src/clocks/clock_scheduler.c behavior
// covers scheduling, capacity, and thread safety

#include <doctest/doctest.h>

#include <atomic>
#include <cfloat>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

extern "C" {
#include "clocks/clock_scheduler.h"
#include "event_types.h"
}

// scheduler runs on background thread. use atomic time control.
static std::atomic<double> g_now{0.0};
static std::atomic<double> g_beats{0.0};

struct PostedEvent {
    event_t type;
    uint32_t thread_id;
    double value;
};

static std::mutex g_ev_mtx;
static std::vector<PostedEvent> g_posted;

// override time stubs with atomic versions for scheduler thread.
extern "C" double jack_client_get_current_time() {
    return g_now.load();
}
extern "C" double clock_get_system_time() {
    return g_now.load();
}
extern "C" double clock_get_beats() {
    return g_beats.load();
}

// override event stubs to capture posted events.
extern "C" union event_data *event_data_new(event_t evcode) {
    auto *ev = (union event_data *)malloc(sizeof(union event_data));
    ev->type = evcode;
    return ev;
}

extern "C" void event_post(union event_data *ev) {
    std::lock_guard<std::mutex> lk(g_ev_mtx);
    if (ev->type == EVENT_CLOCK_RESUME) {
        PostedEvent rec;
        rec.type = static_cast<event_t>(ev->type);
        rec.thread_id = ev->clock_resume.thread_id;
        rec.value = ev->clock_resume.value;
        g_posted.push_back(rec);
    } else {
        PostedEvent rec;
        rec.type = static_cast<event_t>(ev->type);
        rec.thread_id = 0u;
        rec.value = 0.0;
        g_posted.push_back(rec);
    }
    free(ev);
}

static void wait_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

TEST_CASE("scheduler") {
    // setup and teardown for all test cases in this suite
    // start and stop the scheduler thread only once
    clock_scheduler_init();

    SUBCASE("capacity and clear") {
        g_posted.clear();
        clock_scheduler_clear_all();

        bool ok = true;
        for (int i = 0; i < NUM_CLOCK_SCHEDULER_EVENTS; ++i) {
            ok &= clock_scheduler_schedule_sync(i, 1.0, 0.0);
        }
        CHECK(ok == true);

        // scheduler is full. next schedule fails.
        CHECK(clock_scheduler_schedule_sync(9999, 1.0, 0.0) == false);

        clock_scheduler_clear_all();
        CHECK(clock_scheduler_schedule_sync(9999, 1.0, 0.0) == true);
    }

    SUBCASE("sleep posts resume after time elapses") {
        g_posted.clear();
        g_now = 0.0;
        clock_scheduler_clear_all();

        CHECK(clock_scheduler_schedule_sleep(7, 0.05) == true);

        // advance time beyond 50ms and wait for background thread to process.
        g_now = 0.06;
        wait_ms(20);

        std::lock_guard<std::mutex> lk(g_ev_mtx);
        bool seen = false;
        for (const auto &e : g_posted) {
            if (e.type == EVENT_CLOCK_RESUME && e.thread_id == 7) {
                seen = true;
            }
        }
        CHECK(seen == true);
    }

    SUBCASE("sync posts resume after passing next grid beat") {
        g_posted.clear();
        g_now = 0.0;
        g_beats = 0.0;
        clock_scheduler_clear_all();

        CHECK(clock_scheduler_schedule_sync(5, 4.0, 0.0) == true);

        // advance beats past 4.0 and wait for processing.
        g_beats = 4.1;
        wait_ms(20);

        std::lock_guard<std::mutex> lk(g_ev_mtx);
        bool seen = false;
        for (const auto &e : g_posted) {
            if (e.type == EVENT_CLOCK_RESUME && e.thread_id == 5) {
                seen = true;
            }
        }
        CHECK(seen == true);
    }

    SUBCASE("clear all is thread-safe with active background thread") {
        g_posted.clear();
        g_now = 0.0;
        clock_scheduler_clear_all();

        // schedule multiple sleep events with short durations
        CHECK(clock_scheduler_schedule_sleep(1, 0.010) == true);
        CHECK(clock_scheduler_schedule_sleep(2, 0.015) == true);
        CHECK(clock_scheduler_schedule_sleep(3, 0.020) == true);

        // clear all events and wait for background thread to observe cleared state.
        clock_scheduler_clear_all();
        wait_ms(5);

        // advance time beyond all scheduled events and wait for background thread.
        g_now = 0.030;
        wait_ms(50);

        // confirm no events posted after clear_all.
        std::lock_guard<std::mutex> lk(g_ev_mtx);
        CHECK(g_posted.empty());
    }

    SUBCASE("reschedule uses current clock beat") {
        g_posted.clear();
        g_now = 0.0;
        g_beats = 0.0;
        clock_scheduler_clear_all();

        // initial schedule at 4.0 beats
        REQUIRE(clock_scheduler_schedule_sync(10, 4.0, 0.0) == true);

        // move clock to boundary and reschedule. target advances to 8.0.
        g_beats = 4.0;
        clock_scheduler_reschedule_sync_events();

        // advance past 4.0. event does not post due to reschedule.
        g_beats = 4.1;
        wait_ms(20);
        {
            std::lock_guard<std::mutex> lk(g_ev_mtx);
            bool seen = false;
            for (const auto &e : g_posted) {
                if (e.type == EVENT_CLOCK_RESUME && e.thread_id == 10) {
                    seen = true;
                }
            }
            CHECK(seen == false);
        }

        // advance past 8.0. event posts.
        g_beats = 8.1;
        wait_ms(30);
        {
            std::lock_guard<std::mutex> lk(g_ev_mtx);
            bool seen = false;
            for (const auto &e : g_posted) {
                if (e.type == EVENT_CLOCK_RESUME && e.thread_id == 10) {
                    seen = true;
                }
            }
            CHECK(seen == true);
        }
    }

    SUBCASE("reset sets sync_clock_beat to 0 for ready sync events") {
        g_posted.clear();
        g_now = 0.0;
        g_beats = 0.5; // > 0 to trigger immediate eligibility on reset
        clock_scheduler_clear_all();

        REQUIRE(clock_scheduler_schedule_sync(11, 4.0, 0.0) == true);

        // reset sets stored target to 0, causing immediate post on next tick.
        clock_scheduler_reset_sync_events();
        wait_ms(20);

        std::lock_guard<std::mutex> lk(g_ev_mtx);
        bool seen = false;
        for (const auto &e : g_posted) {
            if (e.type == EVENT_CLOCK_RESUME && e.thread_id == 11) {
                seen = true;
            }
        }
        CHECK(seen == true);
    }

    SUBCASE("rescheduling same thread counts from stored sync beat") {
        g_posted.clear();
        g_now = 0.0;
        g_beats = 0.0;
        clock_scheduler_clear_all();

        // schedule at 4.0 and reschedule same thread. target becomes 8.0.
        REQUIRE(clock_scheduler_schedule_sync(12, 4.0, 0.0) == true);
        REQUIRE(clock_scheduler_schedule_sync(12, 4.0, 0.0) == true);

        // crossing 4.0 does not post. counts from stored next beat.
        g_beats = 4.1;
        wait_ms(20);
        {
            std::lock_guard<std::mutex> lk(g_ev_mtx);
            bool seen = false;
            for (const auto &e : g_posted) {
                if (e.type == EVENT_CLOCK_RESUME && e.thread_id == 12) {
                    seen = true;
                }
            }
            CHECK(seen == false);
        }

        // crossing 8.0 posts event.
        g_beats = 8.1;
        wait_ms(30);
        {
            std::lock_guard<std::mutex> lk(g_ev_mtx);
            bool seen = false;
            for (const auto &e : g_posted) {
                if (e.type == EVENT_CLOCK_RESUME && e.thread_id == 12) {
                    seen = true;
                }
            }
            CHECK(seen == true);
        }
    }

    SUBCASE("next-beat boundary math (epsilon/ceil)") {
        // on boundary advances to next grid (4.0 → 8.0 for 4-beat grid).
        CHECK(clock_scheduler_test_next_clock_beat(4.0, 4.0, 0.0) == doctest::Approx(8.0));

        // within epsilon of boundary advances to next grid.
        double near = 4.0 - (FLT_EPSILON * 0.5);
        CHECK(clock_scheduler_test_next_clock_beat(near, 4.0, 0.0) == doctest::Approx(8.0));

        // beyond epsilon rounds to current grid (4.0).
        double below = 4.0 - (FLT_EPSILON * 10.0);
        CHECK(clock_scheduler_test_next_clock_beat(below, 4.0, 0.0) == doctest::Approx(4.0));

        // offset applied correctly (0 → 1.5 with 4.0 beat + 1.5 offset).
        CHECK(clock_scheduler_test_next_clock_beat(0.0, 4.0, 1.5) == doctest::Approx(5.5));
    }

    // teardown
    clock_scheduler_stop();
    clock_scheduler_join();
}