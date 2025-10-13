// tests for crone/src/Window.h and Window.cpp behavior
// covers raised cosine window lookup table used for envelope shaping

#include <algorithm>
#include <cmath>
#include <doctest/doctest.h>

#include "Window.h"

using namespace crone;

TEST_CASE("raisedCosShort: size and value validity") {
    // size must match declared length
    constexpr size_t sz = sizeof(Window::raisedCosShort) / sizeof(Window::raisedCosShort[0]);
    CHECK(sz == Window::raisedCosShortLen);

    // all values finite and within [0,1]
    CHECK(std::all_of(&Window::raisedCosShort[0],
                      &Window::raisedCosShort[0] + Window::raisedCosShortLen,
                      [](float v) { return std::isfinite(v) && v >= 0.0f && v <= 1.0f; }));
}

TEST_CASE("raisedCosShort: shape monotonic and mean around 0.5") {
    // starts at 0, ends at 1
    CHECK(Window::raisedCosShort[0] == doctest::Approx(0.0f));
    CHECK(Window::raisedCosShort[Window::raisedCosShortLen - 1] == doctest::Approx(1.0f));

    // monotonic non-decreasing across table
    const float *tab = &Window::raisedCosShort[0];
    for (size_t i = 0; i + 1 < Window::raisedCosShortLen; ++i) {
        CHECK(tab[i] <= tab[i + 1] + 1e-7f);
    }

    // mean near 0.5
    double sum = 0.0;
    for (size_t i = 0; i < Window::raisedCosShortLen; ++i) {
        sum += Window::raisedCosShort[i];
    }
    double mean = sum / static_cast<double>(Window::raisedCosShortLen);
    CHECK(mean == doctest::Approx(0.5).epsilon(0.05));
}
