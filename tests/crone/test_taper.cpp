// tests for crone/src/Taper.h and Taper.cpp behavior
// covers VU meter position mapping using real ampPosTable from Taper.cpp

#include <doctest/doctest.h>

#include "Taper.h"

using namespace crone;

// -----------------------------------------------------------------------------
// Taper::Vu::getPos range tests

TEST_CASE("Taper::Vu::getPos returns values in [0,1]") {
    // sample across amplitude range to verify output stays within bounds
    for (float amp = 0.0f; amp <= 1.0f; amp += 0.05f) {
        float pos = Taper::Vu::getPos(amp);
        CHECK(pos >= 0.0f);
        CHECK(pos <= 1.0f);
    }
}

TEST_CASE("Taper::Vu::getPos saturates for amplitude > 1") {
    // clipping inputs return maximum position
    CHECK(Taper::Vu::getPos(1.5f) == 1.0f);
    CHECK(Taper::Vu::getPos(2.0f) == 1.0f);
    CHECK(Taper::Vu::getPos(100.0f) == 1.0f);
}

// -----------------------------------------------------------------------------
// Taper::Vu::getPos monotonicity tests

TEST_CASE("Taper::Vu::getPos is monotonic non-decreasing") {
    // position should never decrease as amplitude increases
    float prev = Taper::Vu::getPos(0.0f);
    for (float amp = 0.0f; amp <= 1.0f; amp += 0.01f) {
        float pos = Taper::Vu::getPos(amp);
        CHECK(pos >= prev);
        prev = pos;
    }
}

// -----------------------------------------------------------------------------
// Taper::Vu::getPos anchor point tests

TEST_CASE("Taper::Vu::getPos anchor points match IEC-60268 taper") {
    // validate representative points against actual lookup table values
    // IEC-60268 perceptual taper maps amplitude to VU meter position

    // amp=0.1 interpolates table[3]=0.4906 and table[4]=0.5336
    CHECK(Taper::Vu::getPos(0.1f) == doctest::Approx(0.494865f).epsilon(1e-4f));

    // amp=0.5 interpolates table[15]=0.8113 and table[16]=0.8231
    CHECK(Taper::Vu::getPos(0.5f) == doctest::Approx(0.817191f).epsilon(1e-4f));

    // amp=0.9 interpolates table[27]=0.9528 and table[28]=0.9646
    CHECK(Taper::Vu::getPos(0.9f) == doctest::Approx(0.963438f).epsilon(1e-4f));
}

// -----------------------------------------------------------------------------
// Taper::Vu::getPos endpoint tests

TEST_CASE("Taper::Vu::getPos endpoints match table bounds") {
    // zero amplitude maps to zero position
    CHECK(Taper::Vu::getPos(0.0f) == 0.0f);

    // unity amplitude maps to unity position
    CHECK(Taper::Vu::getPos(1.0f) == 1.0f);
}
