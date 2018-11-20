//
// Created by ezra on 11/9/18.
//

#ifndef SOFTCUT_SOFTCLIP_H
#define SOFTCUT_SOFTCLIP_H

#include <cmath>
#include <boost/math/special_functions/sign.hpp>

namespace softcut {

    // two-stage quadratic soft clipper with variable gain
    // nice odd harmonics, kinda carbon-mic sound
    class SoftClip {
    private:

        float t; // threshold (beginning of knee)
        float g;  // gain multiplier
        float a;  // parabolic coefficient
        float b;  // parabolic offset ( = max level)

        // update quad multiplier from current settings
        void calcCoeffs() {
            // match derivative at knee point
            // FIXME: should be able to factor this such that b=1 always, user sets g only
            float t_1 = t - 1.f;
            if (t_1 < 0.f) {
                a = g / (2.f * t_1);
                b = g * t - (a * t_1 * t_1);
            } else {
                a = 0.f;
                b = 1.f;
            }
        }

    public:
        SoftClip(float t_ = 0.68f, float g_ = 1.2f)
        : t(t_), g(g_) {
            calcCoeffs();
        }

        float processSample(float x) {
            float ax = fabs(x);
            const float sx = boost::math::sign(x);

            if (ax > 1.f) {
                ax = 1.f;
            }

            if (ax < t) {
                return x * g;
            } else {
                const float q = ax - 1.f;
                const float y = (a * q * q) + b;
                return sx * y;
            }
        }

        void setGain(float r) {
            g = r;
            calcCoeffs();
        }

        void setLowThresh(float amp) {
            t = amp;
            calcCoeffs();
        }

        float getGain() { return g; }

        float getLowThresh() { return t; }

        float getHighThreshDb() { return b; }

    };
}

#endif //SOFTCUT_SOFTCLIP_H
