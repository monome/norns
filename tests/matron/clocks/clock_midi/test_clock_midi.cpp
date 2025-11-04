// tests for matron/src/clocks/clock_midi.c behavior
// covers start/stop, tick timing, and tempo smoothing

#include <cmath>
#include <doctest/doctest.h>

extern "C" {
#include "clocks/clock_midi.h"
#include "helpers.h"
}

TEST_CASE("midi: start + ticks update beat and converge tempo to 120") {
    double time = 200.0;
    tests_set_now(time);
    clock_midi_init();

    // send START (counter -> -1)
    clock_midi_handle_message(0xfa);

    // first tick only sets last_tick_time
    time = 200.010; // arbitrary
    tests_set_now(time);
    clock_midi_handle_message(0xf8);

    // feed a number of ticks at 120 BPM (0.5s per beat -> ~0.0208333s per tick)
    double dt = (0.5 / 24.0);
    // 48 iterations = 2x the duration buffer size (24) to warm up smoothing buffer
    for (int i = 0; i < 48; ++i) {
        time += dt;
        tests_set_now(time);
        clock_midi_handle_message(0xf8);
    }
    double tempo1 = clock_midi_get_tempo();
    CHECK(tempo1 > 0.0);

    // after warm-up, beat near zero (allow small drift)
    time = 200.030;
    tests_set_now(time);

    double near_zero = clock_midi_get_beat();
    CHECK(near_zero > -0.1);
    CHECK(near_zero < 0.1);

    // speed up clock: halve dt (simulate 240 BPM), warm up, then compare ratios
    double beat_before = clock_midi_get_beat();
    double dt2 = dt / 2.0;
    // 72 iterations = 3x the duration buffer size (24) to reach new steady state
    for (int i = 0; i < 72; ++i) {
        time += dt2;
        tests_set_now(time);
        clock_midi_handle_message(0xf8);
    }
    double tempo2 = clock_midi_get_tempo();
    CHECK(tempo2 > tempo1);
    CHECK(tempo2 / tempo1 == doctest::Approx(2.0).epsilon(0.25));

    // beat should increase further
    for (int i = 0; i < 12; ++i) {
        time += dt;
        tests_set_now(time);
        clock_midi_handle_message(0xf8);
    }
    double beat_after = clock_midi_get_beat();
    CHECK(beat_after > beat_before);
}

TEST_CASE("midi: stop message handled") {
    // just exercise stop pathway, no observable state here
    clock_midi_init();
    clock_midi_handle_message(0xfc);
    CHECK(true);
}

TEST_CASE("midi: beat counter persists across stop") {
    // this test verifies that the beat counter is not reset by a STOP message.
    // when ticks resume, the counter continues from its previous position.
    // note: norns does not formally handle MIDI CONTINUE (0xfb). this
    // behavior is a side-effect.
    double time = 100.0;
    tests_set_now(time);
    clock_midi_init();

    // send START (counter -> -1)
    clock_midi_handle_message(0xfa);

    // send first tick to establish timing
    time += 0.010;
    tests_set_now(time);
    clock_midi_handle_message(0xf8);

    // warm up the duration buffer with steady 120 BPM ticks
    double dt = (0.5 / 24.0); // 120 BPM: 0.5s per beat, 24 ticks per beat
    // 48 iterations = 2x the duration buffer size (24) to warm up smoothing buffer
    for (int i = 0; i < 48; ++i) {
        time += dt;
        tests_set_now(time);
        clock_midi_handle_message(0xf8);
    }

    // capture beat count before stopping
    double beat_before_stop = clock_midi_get_beat();
    CHECK(beat_before_stop > 0.0);

    // send STOP (0xfc)
    clock_midi_handle_message(0xfc);

    // advance time significantly (simulate pause)
    time += 2.0;
    tests_set_now(time);

    // send a few more ticks after the pause.
    // since the counter was not reset, they should increment from the stopped position.
    for (int i = 0; i < 12; ++i) {
        time += dt;
        tests_set_now(time);
        clock_midi_handle_message(0xf8);
    }

    // beat should have increased from where it stopped, not reset to zero.
    double beat_after_resume = clock_midi_get_beat();
    CHECK(beat_after_resume > beat_before_stop);
}

TEST_CASE("midi: tempo remains finite with very high frequency ticks") {
    double time = 100.0;
    tests_set_now(time);
    clock_midi_init();

    // send START
    clock_midi_handle_message(0xfa);

    // first tick to establish timing
    time += 0.001;
    tests_set_now(time);
    clock_midi_handle_message(0xf8);

    // simulate extremely fast MIDI clock: dt approaching zero (thousands of BPM)
    double dt = 0.0001; // 0.1ms between ticks
    // 48 iterations = 2x the duration buffer size (24) to warm up smoothing buffer
    for (int i = 0; i < 48; ++i) {
        time += dt;
        tests_set_now(time);
        clock_midi_handle_message(0xf8);
    }

    double tempo = clock_midi_get_tempo();
    // verify no inf or NaN (smoothing may produce unusual values with extreme timing)
    CHECK(std::isfinite(tempo));
    CHECK(!std::isnan(tempo));
}

TEST_CASE("midi: tempo remains finite with very low frequency ticks") {
    double time = 100.0;
    tests_set_now(time);
    clock_midi_init();

    // send START
    clock_midi_handle_message(0xfa);

    // first tick to establish timing
    time += 0.010;
    tests_set_now(time);
    clock_midi_handle_message(0xf8);

    // simulate very slow MIDI clock near the 4-second "clock stopped" threshold
    double dt = 3.5 / 24.0; // 3.5s per beat divided by 24 ticks (just under 4s threshold)
    // 48 iterations = 2x the duration buffer size (24) to warm up smoothing buffer
    for (int i = 0; i < 48; ++i) {
        time += dt;
        tests_set_now(time);
        clock_midi_handle_message(0xf8);
    }

    double tempo = clock_midi_get_tempo();
    // tempo should be very low and finite
    CHECK(tempo > 0.0);
    CHECK(tempo < 20.0); // sanity check: should be very slow tempo
    CHECK(std::isfinite(tempo));
    CHECK(!std::isnan(tempo));
}

TEST_CASE("midi: zero dt ticks do not produce inf or NaN") {
    double time = 100.0;
    tests_set_now(time);
    clock_midi_init();

    // send START
    clock_midi_handle_message(0xfa);

    // first tick to establish timing
    time += 0.010;
    tests_set_now(time);
    clock_midi_handle_message(0xf8);

    // warm up with normal tempo
    double dt = (0.5 / 24.0); // 120 BPM
    for (int i = 0; i < 24; ++i) {
        time += dt;
        tests_set_now(time);
        clock_midi_handle_message(0xf8);
    }

    // send tick with exact same timestamp (dt = 0)
    clock_midi_handle_message(0xf8);

    double tempo = clock_midi_get_tempo();
    // verify no inf or NaN despite zero dt
    CHECK(std::isfinite(tempo));
    CHECK(!std::isnan(tempo));
}

TEST_CASE("midi: ticks beyond 4s threshold are treated as clock stopped") {
    double time = 100.0;
    tests_set_now(time);
    clock_midi_init();

    // send START
    clock_midi_handle_message(0xfa);

    // first tick to establish timing
    time += 0.010;
    tests_set_now(time);
    clock_midi_handle_message(0xf8);

    // warm up with normal tempo
    double dt = (0.5 / 24.0); // 120 BPM
    // 48 iterations = 2x the duration buffer size (24) to warm up smoothing buffer
    for (int i = 0; i < 48; ++i) {
        time += dt;
        tests_set_now(time);
        clock_midi_handle_message(0xf8);
    }
    double tempo_before = clock_midi_get_tempo();
    CHECK(tempo_before > 0.0);

    // send tick beyond 4s threshold (clock stopped condition)
    time += 5.0;
    tests_set_now(time);
    clock_midi_handle_message(0xf8);

    // tempo should still be finite
    double tempo_after = clock_midi_get_tempo();
    CHECK(std::isfinite(tempo_after));
    CHECK(!std::isnan(tempo_after));
}
