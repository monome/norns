// tests for crone/src/PeakMeter.h behavior
// covers peak detection, decay, and VU meter position mapping

#include <doctest/doctest.h>

#include "PeakMeter.h"

using namespace crone;

TEST_CASE("PeakMeter rises immediately to peak") {
    // meter captures maximum absolute value in block instantly
    PeakMeter m;
    float block[4] = {0.1f, -0.75f, 0.2f, 0.3f};
    m.update(block, 4);
    CHECK(doctest::Approx(m.get()).epsilon(1e-6) == 0.75f);
}

TEST_CASE("PeakMeter falls smoothly from previous peak") {
    // meter decays gradually when new input is below previous peak
    PeakMeter m;
    float block_hi[2] = {0.75f, 0.1f};
    m.update(block_hi, 2);
    CHECK(m.get() == doctest::Approx(0.75f));

    float block_lo[2] = {0.25f, 0.0f};
    m.update(block_lo, 2);
    const float v = m.get();
    CHECK(v < 0.75f);
    CHECK(v > 0.25f);
}

TEST_CASE("PeakMeter non-negative and no spurious rise after high input") {
    // negative values converted to absolute, meter never rises without higher input
    PeakMeter m;
    float neg_block[3] = {-0.1f, -0.2f, -0.3f};
    m.update(neg_block, 3);
    CHECK(m.get() >= 0.0f);

    float high_block[3] = {1.2f, 2.0f, -1.5f};
    m.update(high_block, 3);
    float peak = m.get();

    float lower_block[3] = {0.9f, 0.8f, 0.7f};
    m.update(lower_block, 3);
    CHECK(m.get() <= peak);
}

TEST_CASE("PeakMeter decays under sustained zero input") {
    // meter gradually returns to zero when fed silence
    PeakMeter m;
    float hi[2] = {0.8f, 0.7f};
    m.update(hi, 2);
    float prev = m.get();
    CHECK(prev > 0.0f);

    float z[4] = {0.f, 0.f, 0.f, 0.f};
    for (int i = 0; i < 8; ++i) {
        m.update(z, 4);
        float v = m.get();
        CHECK(v <= prev);
        CHECK(v >= 0.0f);
        prev = v;
    }
}

TEST_CASE("PeakMeter remains at zero for only zero input") {
    // meter stays at zero when only fed silence
    PeakMeter m;
    float z[8] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    for (int i = 0; i < 4; ++i) {
        m.update(z, 8);
        CHECK(m.get() == doctest::Approx(0.f));
    }
}

TEST_CASE("PeakMeter getPos in [0,1] and increases with level") {
    // VU position maps amplitude to normalized [0,1] range for display
    PeakMeter m;

    float z[4] = {0.f, 0.f, 0.f, 0.f};
    m.update(z, 4);
    CHECK(m.getPos() == doctest::Approx(0.f));

    float mid[4] = {0.5f, 0.4f, 0.3f, 0.2f};
    m.update(mid, 4);
    float pos_mid = m.getPos();
    CHECK(pos_mid >= 0.0f);
    CHECK(pos_mid <= 1.0f);

    float hi[2] = {1.0f, 0.9f};
    m.update(hi, 2);
    float pos_hi = m.getPos();
    CHECK(pos_hi >= pos_mid);
    CHECK(pos_hi == doctest::Approx(1.0f));
}
