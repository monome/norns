//
// Created by ezra on 12/8/17.
//

#ifndef SOFTCUT_INTERPOLATE_H
#define SOFTCUT_INTERPOLATE_H

namespace softcut {
    class Interpolate {
    public:
        template<typename T>
        static inline T hermite(T x, T y0, T y1, T y2, T y3) {
            // 4-point, 3rd-order Hermite (x-form)
#if 0
            T c0 = y1;
            T c1 = 0.5 * (y2 - y0);
            T c2 = y0 - 2.5 * y1 + 2. * y2 - 0.5 * y3;
            T c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
            return ((c3 * x + c2) * x + c1) * x + c0;
#else // inlined:
            return (((0.5 * (y3 - y0) + 1.5 * (y1 - y2)) * x + (y0 - 2.5 * y1 + 2. * y2 - 0.5 * y3)) * x + 0.5 * (y2 - y0)) * x + y1;
#endif
        }

        // super-simple interpolation into a table.
        // this makes assumptions for speed:
        // - allocated table size is >= N+1
        // - index is in [0, 1]
        template<typename T, int N>
        static inline T tabLinear(T* buf, float x) {
            // FIXME: tidy/speed
            const float fi = x * (N-2);
            auto i = static_cast<unsigned int>(fi);
            const float a = buf[i];
            const float b = buf[i+1];
            const float c = (fi - static_cast<float>(i));
            return a + c*(b-a);
        }

    };
}

#endif //SOFTCUT_INTERPOLATE_H
