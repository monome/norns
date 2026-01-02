// tests for crone/src/Utilities.h behavior
// covers denormal filtering (zapgremlins), smoothing, ramping, and lookup tables

#include <cmath>
#include <doctest/doctest.h>
#include <limits>

#include "Utilities.h"

using namespace crone;

// -----------------------------------------------------------------------------
// zapgremlins

TEST_CASE("zapgremlins clamps extreme values and passes normals") {
    CHECK(zapgremlins(0.0f) == 0.0f);
    CHECK(zapgremlins(1.0f) == doctest::Approx(1.0f));
    CHECK(zapgremlins(-1.0f) == doctest::Approx(-1.0f));

    CHECK(zapgremlins(1e-16f) == 0.0f);
    CHECK(zapgremlins(1e16f) == 0.0f);
}

TEST_CASE("zapgremlins zaps NaN and infinities") {
    float nan = std::numeric_limits<float>::quiet_NaN();
    float inf = std::numeric_limits<float>::infinity();
    CHECK(zapgremlins(nan) == 0.0f);
    CHECK(zapgremlins(inf) == 0.0f);
    CHECK(zapgremlins(-inf) == 0.0f);
}

TEST_CASE("zapgremlins clamps exactly at numeric thresholds") {
    const float too_small_boundary = 1e-15f;
    const float too_large_boundary = 1e15f;
    CHECK(zapgremlins(too_small_boundary) == 0.0f);
    CHECK(zapgremlins(-too_small_boundary) == 0.0f);
    CHECK(zapgremlins(too_large_boundary) == 0.0f);
    CHECK(zapgremlins(-too_large_boundary) == 0.0f);
}

// -----------------------------------------------------------------------------
// tau2pole

TEST_CASE("tau2pole yields coefficient in (0,1)") {
    const float t = 0.1f;
    const float sr = 48000.0f;
    float b = tau2pole(t, sr);
    CHECK(b > 0.0f);
    CHECK(b < 1.0f);
}

TEST_CASE("tau2pole approaches 0 for very small time and 1 for large time") {
    const float sr = 48000.0f;
    float b_small_t = tau2pole(1e-6f, sr);
    float b_large_t = tau2pole(10.0f, sr);
    CHECK(b_small_t < 1e-6f);
    CHECK(b_large_t > 0.99f);
}

TEST_CASE("tau2pole increases with sample rate for fixed time") {
    const float t = 0.1f;
    float b_sr_low = tau2pole(t, 1000.0f);
    float b_sr_high = tau2pole(t, 48000.0f);
    CHECK(b_sr_high > b_sr_low);
}

// -----------------------------------------------------------------------------
// smooth1pole

TEST_CASE("smooth1pole edge cases b=0 and b=1") {
    const float x = 1.0f;
    const float x0 = 0.0f;
    CHECK(smooth1pole(x, x0, 0.0f) == doctest::Approx(1.0f)); // pass-through
    CHECK(smooth1pole(x, x0, 1.0f) == doctest::Approx(0.0f)); // hold previous
}

TEST_CASE("smooth1pole output stays within input and previous bounds") {
    const float x = 1.0f;
    const float x0 = 0.0f;
    float y1 = smooth1pole(x, x0, 0.25f);
    float y2 = smooth1pole(x, x0, 0.75f);
    CHECK(y1 >= 0.0f);
    CHECK(y1 <= 1.0f);
    CHECK(y2 >= 0.0f);
    CHECK(y2 <= 1.0f);
}

// -----------------------------------------------------------------------------
// running average

TEST_CASE("RunningAverage zero-padding then steady-state under-read") {
    RunningAverage<float, 4> avg;
    CHECK(avg.update(1.0f) == doctest::Approx(0.25f));
    for (int i = 0; i < 8; ++i)
        avg.update(1.0f);
    CHECK(avg.update(1.0f) == doctest::Approx(0.75f));
}

TEST_CASE("RunningAverage tracks moving average over window size") {
    RunningAverage<float, 4> avg;
    for (int i = 0; i < 4; ++i) {
        avg.update(0.0f);
    }
    CHECK(avg.update(1.0f) == doctest::Approx(0.25f));
    CHECK(avg.update(2.0f) == doctest::Approx(0.75f));
    CHECK(avg.update(3.0f) == doctest::Approx(1.5f));
    CHECK(avg.update(4.0f) == doctest::Approx(2.25f));
}

TEST_CASE("RunningAverage handles negative values") {
    RunningAverage<float, 4> avg;
    for (int i = 0; i < 3; ++i) {
        avg.update(-1.0f);
    }
    float mean = avg.update(-1.0f);
    CHECK(mean == doctest::Approx(-0.75f));
}

// -----------------------------------------------------------------------------
// linear ramp

TEST_CASE("LinearRamp approaches target monotonically without overshoot") {
    LinearRamp ramp(200.0f, 0.5f);
    float prev = 0.0f;
    for (int i = 0; i < 50; ++i) {
        float y = ramp.process(1.0f);
        CHECK(y >= prev);
        CHECK(y <= 1.0f);
        prev = y;
    }
}

TEST_CASE("LinearRamp clamps when step would overshoot") {
    LinearRamp ramp(100.0f, 0.0001f);
    CHECK(ramp.process(1.0f) == doctest::Approx(1.0f));
    CHECK(ramp.process(0.0f) == doctest::Approx(0.0f));
}

TEST_CASE("LinearRamp downward ramp approaches target monotonically") {
    LinearRamp ramp(200.0f, 0.5f);
    for (int i = 0; i < 50; ++i) {
        ramp.process(1.0f);
    }
    float prev = 1.0f;
    for (int i = 0; i < 50; ++i) {
        float current = ramp.process(0.0f);
        CHECK(current <= prev);
        CHECK(current >= 0.0f);
        prev = current;
    }
}

TEST_CASE("LinearRamp downward step clamps at target") {
    LinearRamp ramp(1.0f, 0.001f);
    ramp.process(0.5f);
    float result = ramp.process(0.0f);
    CHECK(result == doctest::Approx(0.0));
    CHECK(result >= 0.0f);
}

TEST_CASE("LinearRamp setSampleRate mid-ramp recomputes increment") {
    LinearRamp ramp(10.0f, 1.0f);
    float first = ramp.process(1.0f);
    CHECK(first == doctest::Approx(0.1f));
    ramp.setSampleRate(20.0f);
    float second = ramp.process(1.0f);
    // remaining 0.9, new inc = 0.9 / (1 * 20) = 0.045
    CHECK(second == doctest::Approx(0.145f));
}

TEST_CASE("LinearRamp setTime recomputes increment") {
    LinearRamp ramp(10.0f, 1.0f);
    float first = ramp.process(1.0f);
    CHECK(first == doctest::Approx(0.1));
    ramp.setTime(0.5f);
    float second = ramp.process(1.0f);
    // remaining 0.9, new inc = 0.9 / (0.5 * 10) = 0.18
    CHECK(second == doctest::Approx(0.1 + 0.18));
}

TEST_CASE("LinearRamp doubling time mid-ramp halves increment") {
    LinearRamp ramp(10.0f, 1.0f);
    CHECK(ramp.process(1.0f) == doctest::Approx(0.1f));
    ramp.setTime(2.0f);
    CHECK(ramp.process(1.0f) == doctest::Approx(0.145f));
}

TEST_CASE("LinearRamp approaches negative target monotonically") {
    LinearRamp ramp(200.0f, 0.5f);
    float prev = 0.0f;
    for (int i = 0; i < 50; ++i) {
        float y = ramp.process(-1.0f);
        CHECK(y <= prev);
        CHECK(y >= -1.0f);
        prev = y;
    }
}

// -----------------------------------------------------------------------------
// log ramp

TEST_CASE("LogRamp rising monotonic and bounded") {
    LogRamp ramp(1000.0f, 0.01f);
    float prev = 0.0f;
    for (int i = 0; i < 100; ++i) {
        float y = ramp.process(1.0f);
        CHECK(y >= prev);
        CHECK(y <= 1.0f);
        prev = y;
    }
}

TEST_CASE("LogRamp falling monotonic and bounded") {
    LogRamp ramp(1000.0f, 0.01f);
    for (int i = 0; i < 100; ++i)
        ramp.process(1.0f);
    float prev = 1.0f;
    for (int i = 0; i < 100; ++i) {
        float y = ramp.process(0.0f);
        CHECK(y <= prev);
        CHECK(y >= 0.0f);
        prev = y;
    }
}

TEST_CASE("LogRamp setSampleRate mid-ramp stretches time without discontinuity") {
    LogRamp ramp(1000.0f, 0.05f);
    float before = 0.0f;
    for (int i = 0; i < 5; ++i) {
        before = ramp.process(1.0f);
    }
    ramp.setSampleRate(2000.0f);
    float after = ramp.process(1.0f);
    CHECK(after >= before); // still moving toward target
    CHECK(after <= 1.0f);   // bounded by target
}

TEST_CASE("LogRamp setTime mid-ramp adjusts convergence speed smoothly") {
    LogRamp ramp(1000.0f, 0.05f);
    float y1 = ramp.process(1.0f);
    ramp.setTime(0.10f);
    float y2 = ramp.process(1.0f);
    CHECK(y2 >= y1);
    CHECK(y2 <= 1.0f);
    ramp.setTime(0.01f);
    float y3 = ramp.process(1.0f);
    CHECK(y3 >= y2);
    CHECK(y3 <= 1.0f);
}

TEST_CASE("LogRamp handles negative target with monotonic approach") {
    LogRamp ramp(1000.0f, 0.02f);
    float prev = 0.0f;
    for (int i = 0; i < 100; ++i) {
        float y = ramp.process(-1.0f);
        CHECK(y <= prev);
        CHECK(y >= -1.0f);
        prev = y;
    }
}

TEST_CASE("LogRamp setTarget/getTarget symmetry") {
    LogRamp ramp(1000.0f, 0.05f);
    ramp.setTarget(0.42f);
    CHECK(ramp.getTarget() == doctest::Approx(0.42f));
}

TEST_CASE("LogRamp update() continues toward fixed target") {
    LogRamp ramp(1000.0f, 0.02f);
    ramp.setTarget(1.0f);
    float y1 = ramp.update();
    float y2 = ramp.update();
    float y3 = ramp.update();
    CHECK(y1 >= 0.0f);
    CHECK(y2 >= y1);
    CHECK(y3 >= y2);
    CHECK(y3 <= 1.0f);
}

// -----------------------------------------------------------------------------
// slew

TEST_CASE("Slew faster rise reaches target sooner") {
    Slew fastRise(1000.0f, 0.01f, 0.1f);
    Slew slowRise(1000.0f, 0.1f, 0.1f);
    float yf = 0.0f, ys = 0.0f;
    for (int i = 0; i < 10; ++i) {
        yf = fastRise.process(1.0f);
        ys = slowRise.process(1.0f);
        CHECK(yf >= 0.0f);
        CHECK(ys >= 0.0f);
        CHECK(yf <= 1.0f);
        CHECK(ys <= 1.0f);
    }
    CHECK(yf > ys);
}

TEST_CASE("Slew faster fall decays sooner without undershoot") {
    Slew a(1000.0f, 0.05f, 0.01f);
    Slew b(1000.0f, 0.05f, 0.1f);
    for (int i = 0; i < 100; ++i) {
        a.process(1.0f);
        b.process(1.0f);
    }
    float ya = 1.0f, yb = 1.0f;
    for (int i = 0; i < 10; ++i) {
        ya = a.process(0.0f);
        yb = b.process(0.0f);
        CHECK(ya <= 1.0f);
        CHECK(yb <= 1.0f);
        CHECK(ya >= 0.0f);
        CHECK(yb >= 0.0f);
    }
    CHECK(ya < yb);
}

TEST_CASE("Slew setSampleRate recomputes rise/fall coefficients") {
    Slew s(10.0f, 1.0f, 2.0f);
    float y1 = s.process(1.0f);
    s.setSampleRate(20.0f);
    float y2 = s.process(1.0f);
    CHECK(y2 >= y1);
    CHECK(y2 <= 1.0f);
}

TEST_CASE("Slew extreme small/large times behave reasonably") {
    Slew nearInstant(1000.0f, 1e-6f, 1e-6f);
    float yi = nearInstant.process(1.0f);
    CHECK(yi > 0.9f);
    Slew verySlow(1000.0f, 10.0f, 10.0f);
    float ys = verySlow.process(1.0f);
    CHECK(ys < 0.2f);
}

TEST_CASE("Slew independent rise time slows upward movement") {
    Slew s(1000.0f, 0.1f, 0.1f);
    float before = 0.0f;
    for (int i = 0; i < 8; ++i) {
        before = s.process(1.0f);
    }
    float y_pre = s.process(1.0f);
    float delta_pre = y_pre - before;
    s.setRiseTime(0.5f);
    float y_post = s.process(1.0f);
    float delta_post = y_post - y_pre;
    CHECK(delta_post < delta_pre);
}

// -----------------------------------------------------------------------------
// lookup table

TEST_CASE("LUT::lookupLinear interpolates between points") {
    float tab[2] = {0.0f, 10.0f};
    CHECK(LUT<float>::lookupLinear(0.0f, tab, 2) == doctest::Approx(0.0f));
    CHECK(LUT<float>::lookupLinear(1.0f, tab, 2) == doctest::Approx(10.0f));
    CHECK(LUT<float>::lookupLinear(0.5f, tab, 2) == doctest::Approx(5.0f));
}

TEST_CASE("LUT::lookupLinear clamps x > 1 to last entry") {
    const float tab[5] = {0.0f, 2.0f, 4.0f, 6.0f, 8.0f};
    CHECK(LUT<float>::lookupLinear(1.0f, tab, 5) == doctest::Approx(8.0f));
    CHECK(LUT<float>::lookupLinear(1.25f, tab, 5) == doctest::Approx(8.0f));
}

TEST_CASE("LUT::lookupLinear handles interior fractional indices") {
    const float tab[4] = {0.0f, 10.0f, 20.0f, 30.0f};
    CHECK(LUT<float>::lookupLinear(0.25f, tab, 4) == doctest::Approx(7.5f));
    CHECK(LUT<float>::lookupLinear(0.75f, tab, 4) == doctest::Approx(22.5f));
}

TEST_CASE("LUT::lookupLinear size-1 table returns the only element") {
    const float tab[1] = {3.14f};
    CHECK(LUT<float>::lookupLinear(0.0f, tab, 1) == doctest::Approx(3.14f));
    CHECK(LUT<float>::lookupLinear(0.5f, tab, 1) == doctest::Approx(3.14f));
    CHECK(LUT<float>::lookupLinear(1.0f, tab, 1) == doctest::Approx(3.14f));
}
