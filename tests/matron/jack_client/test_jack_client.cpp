// tests for matron/src/jack_client.c 64-bit time wrap tracking

#include <atomic>
#include <cstdint>
#include <doctest/doctest.h>
#include <thread>

extern "C" {
#include "jack_client.h"
// test-only seam added under NORNS_TEST
void jack_client_test_set_time_state(uint64_t frames);
void jack_client_test_reset_time_state(void);
// global from jack_client.c
extern double jack_sample_rate;
}

// stub JACK frame time source controlled by tests
static std::atomic<uint32_t> g_stub_frames{0u};

extern "C" {
#include "jack/jack.h"

jack_nframes_t jack_frame_time(jack_client_t *) {
    return static_cast<jack_nframes_t>(g_stub_frames.load());
}
}

static void set_sr(double sr) {
    jack_sample_rate = sr;
}

TEST_CASE("jack_client: time progresses forward without wrap") {
    jack_client_test_reset_time_state();
    set_sr(48000.0);

    g_stub_frames = 100u;
    double t1 = jack_client_get_current_time();
    CHECK(t1 == doctest::Approx(100.0 / 48000.0));

    g_stub_frames = 100000u;
    double t2 = jack_client_get_current_time();
    CHECK(t2 == doctest::Approx(100000.0 / 48000.0));
    CHECK(t2 >= t1);
}

TEST_CASE("jack_client: time wrap produces correct 64-bit time") {
    // seed jack client state to pre-wrap value so signed delta sees a small change
    jack_client_test_set_time_state(0xFFFFFFF0u); // 2^32 - 16
    set_sr(48000.0);

    // drive close to 2^32 - 16, then wrap to 32
    g_stub_frames = 0xFFFFFFF0u; // 2^32 - 16
    double t_pre = jack_client_get_current_time();
    CHECK(t_pre == doctest::Approx(4294967280.0 / 48000.0));

    g_stub_frames = 0x00000020u; // 32
    double t_post = jack_client_get_current_time();

    // expected: (2^32 + 32) / sr
    const double expected = (4294967296.0 + 32.0) / 48000.0;
    CHECK(t_post == doctest::Approx(expected));
    CHECK(t_post >= t_pre);
}

TEST_CASE("jack_client: time is monotonic with out-of-order frames") {
    jack_client_test_reset_time_state();
    set_sr(48000.0);

    g_stub_frames = 1000u;
    double t1 = jack_client_get_current_time();
    CHECK(t1 == doctest::Approx(1000.0 / 48000.0));

    // smaller value but not a real wrap pattern (far from range ends)
    g_stub_frames = 900u;
    double t2 = jack_client_get_current_time();
    // monotonic: should not decrease and should not jump by ~2^32 frames
    CHECK(t2 >= t1);
    CHECK(t2 == doctest::Approx(t1));
}

TEST_CASE("jack_client: time is monotonic across threads during wrap") {
    jack_client_test_reset_time_state();
    set_sr(48000.0);

    // prepare to cross wrap boundary while multiple readers observe time
    g_stub_frames = 0xFFFFFF00u; // near top

    // reader samples
    std::atomic<double> r1_last{0.0}, r2_last{0.0};
    std::atomic<int> r1_bad{0}, r2_bad{0};

    auto read_loop = [](std::atomic<double> &last, std::atomic<int> &bad) {
        for (int i = 0; i < 1000; ++i) {
            double t = jack_client_get_current_time();
            double prev = last.load();
            if (t + 1e-12 < prev) {
                ++bad; // detected regression
            } else {
                last.store(t);
            }
        }
    };

    std::thread r1(read_loop, std::ref(r1_last), std::ref(r1_bad));
    std::thread r2(read_loop, std::ref(r2_last), std::ref(r2_bad));

    // advance past wrap while readers are running
    for (uint32_t f = 0xFFFFFF00u; f != 0x00000100u; ++f) {
        g_stub_frames = f;
    }

    r1.join();
    r2.join();

    CHECK(r1_bad.load() == 0);
    CHECK(r2_bad.load() == 0);
}
