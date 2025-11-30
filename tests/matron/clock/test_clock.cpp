// tests for matron/src/clock.h and clock.c behavior
// uses mocks for cross-module calls and C stubs to isolate deps

#include <atomic>
#include <cmath>
#include <doctest/doctest.h>
#include <pthread.h>
#include <trompeloeil.hpp>

extern "C" {
#include "clock.h"
#include "events.h"
}

// -----------------------------------------------------------------------------
// C stubs delegating to a C++ mock for cross-module calls
// allows isolation of clock.c logic from external dependencies
// -----------------------------------------------------------------------------

namespace {
struct ClockDepsMock {
    // clock backends
    MAKE_MOCK0(internal_get_beat, double());
    MAKE_MOCK0(midi_get_beat, double());
    MAKE_MOCK0(link_get_beat, double());
    MAKE_MOCK0(crow_get_beat, double());

    MAKE_MOCK0(internal_get_tempo, double());
    MAKE_MOCK0(midi_get_tempo, double());
    MAKE_MOCK0(link_get_tempo, double());
    MAKE_MOCK0(crow_get_tempo, double());

    // scheduler + events
    MAKE_MOCK0(scheduler_reset_sync_events, void());
    MAKE_MOCK0(scheduler_reschedule_sync_events, void());
    MAKE_MOCK1(event_data_new, union event_data *(event_t));
    MAKE_MOCK1(event_post, void(union event_data *));

    // link session
    MAKE_MOCK0(link_join_session, void());
    MAKE_MOCK0(link_leave_session, void());
    MAKE_MOCK0(link_number_of_peers, uint64_t());
};

static ClockDepsMock *g_mock = nullptr;
static double g_now = 0.0;
} // namespace

extern "C" {
double jack_client_get_current_time() {
    return g_now;
}

// clock backends
double clock_internal_get_beat() {
    return g_mock ? g_mock->internal_get_beat() : 0.0;
}
double clock_midi_get_beat() {
    return g_mock ? g_mock->midi_get_beat() : 0.0;
}
double clock_link_get_beat() {
    return g_mock ? g_mock->link_get_beat() : 0.0;
}
double clock_crow_get_beat() {
    return g_mock ? g_mock->crow_get_beat() : 0.0;
}

double clock_internal_get_tempo() {
    return g_mock ? g_mock->internal_get_tempo() : 0.0;
}
double clock_midi_get_tempo() {
    return g_mock ? g_mock->midi_get_tempo() : 0.0;
}
double clock_link_get_tempo() {
    return g_mock ? g_mock->link_get_tempo() : 0.0;
}
double clock_crow_get_tempo() {
    return g_mock ? g_mock->crow_get_tempo() : 0.0;
}

// scheduler and notifications
void clock_scheduler_reset_sync_events() {
    if (g_mock) {
        g_mock->scheduler_reset_sync_events();
    }
}
void clock_scheduler_reschedule_sync_events() {
    if (g_mock) {
        g_mock->scheduler_reschedule_sync_events();
    }
}
union event_data *event_data_new(event_t evcode) {
    return g_mock ? g_mock->event_data_new(evcode) : nullptr;
}
void event_post(union event_data *ev) {
    if (g_mock) {
        g_mock->event_post(ev);
    }
}

// link session
void clock_link_join_session() {
    if (g_mock) {
        g_mock->link_join_session();
    }
}
void clock_link_leave_session() {
    if (g_mock) {
        g_mock->link_leave_session();
    }
}
uint64_t clock_link_number_of_peers() {
    return g_mock ? g_mock->link_number_of_peers() : 0u;
}
}

// helper to create event_data for mocking event_data_new to match production
// behavior
static union event_data *make_event(event_t type) {
    auto *ev = (union event_data *)calloc(1, sizeof(union event_data));
    ev->type = type;
    return ev;
}

TEST_CASE("clock: init triggers reschedule") {
    ClockDepsMock mock;
    g_mock = &mock;

    REQUIRE_CALL(mock, scheduler_reschedule_sync_events()).TIMES(1);
    ALLOW_CALL(mock, internal_get_tempo()).RETURN(120.0);
    ALLOW_CALL(mock, internal_get_beat()).RETURN(0.0);

    clock_init();
}

TEST_CASE("clock: get system time proxies to jack") {
    ClockDepsMock mock;
    g_mock = &mock;
    g_now = 123.456;
    CHECK(clock_get_system_time() == doctest::Approx(123.456));
}

TEST_CASE("clock: set source join/leave link + reschedule") {
    ClockDepsMock mock;
    g_mock = &mock;

    // init sets internal and reschedules
    ALLOW_CALL(mock, scheduler_reschedule_sync_events());
    clock_init();

    // switch to LINK: join + reschedule
    {
        trompeloeil::sequence seq1;
        REQUIRE_CALL(mock, link_join_session()).IN_SEQUENCE(seq1);
        REQUIRE_CALL(mock, scheduler_reschedule_sync_events()).IN_SEQUENCE(seq1);
        clock_set_source(CLOCK_SOURCE_LINK);
    }

    // switch to MIDI: leave + reschedule
    {
        trompeloeil::sequence seq2;
        REQUIRE_CALL(mock, link_leave_session()).IN_SEQUENCE(seq2);
        REQUIRE_CALL(mock, scheduler_reschedule_sync_events()).IN_SEQUENCE(seq2);
        clock_set_source(CLOCK_SOURCE_MIDI);
    }
}

TEST_CASE("clock: beats and tempo delegate to active source") {
    ClockDepsMock mock;
    g_mock = &mock;

    ALLOW_CALL(mock, scheduler_reschedule_sync_events());
    ALLOW_CALL(mock, link_join_session());
    ALLOW_CALL(mock, link_leave_session());
    clock_init();

    // internal
    REQUIRE_CALL(mock, internal_get_beat()).RETURN(1.25);
    REQUIRE_CALL(mock, internal_get_tempo()).RETURN(110.0);
    clock_set_source(CLOCK_SOURCE_INTERNAL);
    CHECK(clock_get_beats() == doctest::Approx(1.25));
    CHECK(clock_get_tempo() == doctest::Approx(110.0));

    // midi
    REQUIRE_CALL(mock, midi_get_beat()).RETURN(2.5);
    REQUIRE_CALL(mock, midi_get_tempo()).RETURN(98.0);
    clock_set_source(CLOCK_SOURCE_MIDI);
    CHECK(clock_get_beats() == doctest::Approx(2.5));
    CHECK(clock_get_tempo() == doctest::Approx(98.0));

    // link
    REQUIRE_CALL(mock, link_get_beat()).RETURN(8.0);
    REQUIRE_CALL(mock, link_get_tempo()).RETURN(123.0);
    clock_set_source(CLOCK_SOURCE_LINK);
    CHECK(clock_get_beats() == doctest::Approx(8.0));
    CHECK(clock_get_tempo() == doctest::Approx(123.0));

    // crow
    REQUIRE_CALL(mock, crow_get_beat()).RETURN(4.0);
    REQUIRE_CALL(mock, crow_get_tempo()).RETURN(60.0);
    clock_set_source(CLOCK_SOURCE_CROW);
    CHECK(clock_get_beats() == doctest::Approx(4.0));
    CHECK(clock_get_tempo() == doctest::Approx(60.0));
}

TEST_CASE("clock: start/stop/reschedule only when source matches") {
    ClockDepsMock mock;
    g_mock = &mock;

    ALLOW_CALL(mock, scheduler_reschedule_sync_events());
    clock_init();
    clock_set_source(CLOCK_SOURCE_INTERNAL);

    // when source matches, expect reset, event data new, and post
    {
        trompeloeil::sequence seq;
        REQUIRE_CALL(mock, scheduler_reset_sync_events()).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, event_data_new(EVENT_CLOCK_START))
            .IN_SEQUENCE(seq)
            .LR_RETURN(make_event(EVENT_CLOCK_START));
        REQUIRE_CALL(mock, event_post(trompeloeil::_)).IN_SEQUENCE(seq);
        clock_start_from_source(CLOCK_SOURCE_INTERNAL);
    }

    {
        trompeloeil::sequence seq;
        REQUIRE_CALL(mock, event_data_new(EVENT_CLOCK_STOP))
            .IN_SEQUENCE(seq)
            .LR_RETURN(make_event(EVENT_CLOCK_STOP));
        REQUIRE_CALL(mock, event_post(trompeloeil::_)).IN_SEQUENCE(seq);
        clock_stop_from_source(CLOCK_SOURCE_INTERNAL);
    }

    // when source does not match, none of these should be called
    FORBID_CALL(mock, scheduler_reset_sync_events());
    FORBID_CALL(mock, event_data_new(trompeloeil::_));
    FORBID_CALL(mock, event_post(trompeloeil::_));
    clock_start_from_source(CLOCK_SOURCE_MIDI);
    clock_stop_from_source(CLOCK_SOURCE_MIDI);

    // reschedule from source must match
    REQUIRE_CALL(mock, scheduler_reschedule_sync_events()).TIMES(1);
    clock_reschedule_sync_events_from_source(CLOCK_SOURCE_INTERNAL);
    FORBID_CALL(mock, scheduler_reschedule_sync_events());
    clock_reschedule_sync_events_from_source(CLOCK_SOURCE_MIDI);
}

TEST_CASE("clock reference: beat and tempo computation") {
    ClockDepsMock mock;
    g_mock = &mock;

    clock_reference_t ref;

    g_now = 100.0;
    clock_reference_init(&ref);

    // default tempo 120 bpm (0.5s per beat)
    CHECK(clock_get_reference_tempo(&ref) == doctest::Approx(120.0));

    // after 0.25s, beat advanced by 0.5
    g_now = 100.25;
    CHECK(clock_get_reference_beat(&ref) == doctest::Approx(0.5));

    // update reference: set beat to 2.0, duration 1.0s
    g_now = 101.0;
    clock_update_source_reference(&ref, 2.0, 1.0);

    // tempo now 60 bpm
    CHECK(clock_get_reference_tempo(&ref) == doctest::Approx(60.0));

    // after 0.5s, beat advanced by 0.5
    g_now = 101.5;
    CHECK(clock_get_reference_beat(&ref) == doctest::Approx(2.5));
}

TEST_CASE("clock: number of Link peers delegates to link") {
    ClockDepsMock mock;
    g_mock = &mock;
    REQUIRE_CALL(mock, link_number_of_peers()).RETURN(3u);
    CHECK(clock_number_of_link_peers() == 3u);
}

TEST_CASE("clock: set_source is idempotent for same source") {
    ClockDepsMock mock1;
    g_mock = &mock1;

    // allow init reschedule
    ALLOW_CALL(mock1, scheduler_reschedule_sync_events());
    clock_init();

    // switch to MIDI once -> reschedule exactly once across two identical calls
    ClockDepsMock mock2;
    g_mock = &mock2;

    REQUIRE_CALL(mock2, scheduler_reschedule_sync_events()).TIMES(1);
    clock_set_source(CLOCK_SOURCE_MIDI);
    clock_set_source(CLOCK_SOURCE_MIDI);
}

TEST_CASE("clock: non-link transitions do not call link join/leave") {
    ClockDepsMock mock;
    g_mock = &mock;

    // init sets INTERNAL; allow its reschedule
    ALLOW_CALL(mock, scheduler_reschedule_sync_events());
    clock_init();

    // forbid any link session calls for non-link transitions
    FORBID_CALL(mock, link_join_session());
    FORBID_CALL(mock, link_leave_session());

    // INTERNAL -> MIDI -> CROW -> INTERNAL should never touch link
    ALLOW_CALL(mock, scheduler_reschedule_sync_events());
    clock_set_source(CLOCK_SOURCE_MIDI);
    clock_set_source(CLOCK_SOURCE_CROW);
    clock_set_source(CLOCK_SOURCE_INTERNAL);
}

TEST_CASE("clock reference: update reflects immediately in beat/tempo getters") {
    ClockDepsMock mock;
    g_mock = &mock;

    clock_reference_t ref;

    // establish a known time
    g_now = 200.0;
    clock_reference_init(&ref);

    // update the reference and verify immediate values
    clock_update_source_reference(&ref, 10.0, 0.75); // 80 bpm
    CHECK(clock_get_reference_beat(&ref) == doctest::Approx(10.0));
    CHECK(clock_get_reference_tempo(&ref) == doctest::Approx(80.0));

    // after 0.75s, beat advances by 1
    g_now = 200.75;
    CHECK(clock_get_reference_beat(&ref) == doctest::Approx(11.0));
}

TEST_CASE("clock reference: concurrent updates and reads stay thread-safe and monotonic") {
    ClockDepsMock mock;
    g_mock = &mock;

    clock_reference_t ref;
    g_now = 0.0;
    clock_reference_init(&ref);

    // track cross-thread failures: invalid values or position moving backwards
    std::atomic<int> bad_monotonic{0};
    std::atomic<int> bad_value{0};

    pthread_t updater;
    pthread_t reader1;
    pthread_t reader2;

    // updater thread: write new position (in beats) and tempo repeatedly
    auto update_func = +[](void *p) -> void * {
        auto *r = reinterpret_cast<clock_reference_t *>(p);
        for (int i = 0; i < 5000; ++i) {
            g_now = i * 0.001; // advance virtual time monotonically
            // vary tempo to exercise different beat durations
            clock_update_source_reference(r, i * 0.1, 0.5 + (i % 10) * 0.01);
        }
        return nullptr;
    };

    struct reader_args {
        clock_reference_t *ref;
        std::atomic<int> *bad_mono;
        std::atomic<int> *bad_val;
    };

    // reader threads: read position and tempo and check invariants
    auto read_func = +[](void *p) -> void * {
        auto *ctx = reinterpret_cast<reader_args *>(p);
        clock_reference_t *r = ctx->ref;
        std::atomic<int> *bad_mono = ctx->bad_mono;
        std::atomic<int> *bad_val = ctx->bad_val;

        double last_beat = -1.0;
        for (int i = 0; i < 5000; ++i) {
            double b = clock_get_reference_beat(r);
            double t = clock_get_reference_tempo(r);

            // require finite position and strictly positive tempo
            if (!(std::isfinite(b) && t > 0.0)) {
                ++(*bad_val);
            }
            // position should not move backwards (allow tiny epsilon for float error)
            if (last_beat >= 0.0 && b < last_beat - 1e-6) {
                ++(*bad_mono);
            }
            last_beat = b;
        }
        return nullptr;
    };

    reader_args arg{&ref, &bad_monotonic, &bad_value};

    // run one updater and two readers concurrently
    pthread_create(&updater, nullptr, update_func, &ref);
    pthread_create(&reader1, nullptr, read_func, &arg);
    pthread_create(&reader2, nullptr, read_func, &arg);

    pthread_join(updater, nullptr);
    pthread_join(reader1, nullptr);
    pthread_join(reader2, nullptr);

    // verify no failures: values are valid and position remains monotonic
    CHECK(bad_value.load() == 0);
    CHECK(bad_monotonic.load() == 0);
}
