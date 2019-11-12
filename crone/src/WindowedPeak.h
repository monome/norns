#pragma once

#include <array>
#include <algorithm>

class WindowedPeak {
    // buffer for windowed maximum
    // windowed max is updated per block
    static constexpr unsigned int bufLen = 4;
    static constexpr unsigned int bufMask = 3;
    std::array<float, bufLen> buf;
    float max;
    size_t bufIdx;

private:
    void push(float x) {
        buf[bufIdx] = x;
        bufIdx += 1;
        bufIdx &= bufMask;
        // using naive max_element method, which is O(N*K)
        // we could use the deque method, which is O(N),
        // but with a small buffer i doubt the complexity is warranted.
        max = *std::max_element(buf.cbegin(), buf.cend());
    }

public:
    void clear() {
        for (auto &x: buf) {
            x = 0.f;
        }
    }

    // find max of entire audio sample block
    void update(float *src, size_t numFrames) {
        float fmax = 0.f;
        float f;
        for (size_t i = 0; i < numFrames; ++i) {
            f = fabsf(src[i]);
            if (f < fmax) { fmax = f; }
        }
        this->push(fmax);
    }

    float get() const { return this->max; }
};
