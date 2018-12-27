//
// Created by ezra on 4/21/18.
//

#ifndef SOFTCUTHEAD_RESAMPLER_H
#define SOFTCUTHEAD_RESAMPLER_H

#include <iostream>
#include <cmath>

#include <boost/assert.hpp>
#include "Types.h"
#include "Interpolate.h"

// ultra-simple resampling class
// works on mono output buffer and processes one input sample at a time

// uncomment to use linear interpolation. honestly can't hear a huge difference?
// FIXME: should be template param i guess
// #define RESAMPLER_INTERPOLATE_LINEAR

namespace softcut {

    class Resampler {

    public:

        enum {
            IN_BUF_FRAMES = 4, // limits interpolation order
            IN_BUF_MASK = 3,
            OUT_BUF_FRAMES = 64 // limits resampling ratio
        };

        // constructor
        Resampler() : rate_(1.0), phi_(1.0), phase_(0.0)
#ifdef RESAMPLER_INTERPOLATE_LINEAR
#else
                ,inBufIdx_(0)
#endif
        {}

        int processFrame(sample_t x){
            pushInput(x);
            if(rate_ > 1.0) {
                return writeUp();
            } else {
                return writeDown();
            }
        }

        void setRate(rate_t r) {
            rate_ = r;
            phi_ = 1.0 / r;
        }
        // void setBuffer(float *buf, int frames);
        void setPhase(phase_t phase) { phase_ = phase; }
        const sample_t* output(){
            return static_cast<const sample_t*>(outBuf_);
        }

        void reset() {

#ifdef RESAMPLER_INTERPOLATE_LINEAR
            x_ = 0.f;
#else
            for (sample_t &i : inBuf_) { i = 0.f; }
            for (sample_t &i : outBuf_) { i = 0.f; }
            inBufIdx_ = 0;
#endif
        }

    private:
        rate_t rate_;
        // phase increment
        phase_t phi_;
        // last written phase
        phase_t phase_;
#ifdef RESAMPLER_INTERPOLATE_LINEAR
        // current input value
        sample_t x_;
        // last input value
        sample_t x_1_;
#else
        // input ringbuffer
        sample_t inBuf_[IN_BUF_FRAMES];
        unsigned int inBufIdx_;
#endif
        // output buffer
        sample_t outBuf_[OUT_BUF_FRAMES];

    private:
        // push an input value
        void pushInput(sample_t x){
#ifdef RESAMPLER_INTERPOLATE_LINEAR
            x_1_ = x_;
            x_ = x;
#else
            inBufIdx_ = (inBufIdx_ + 1) & IN_BUF_MASK;
            inBuf_[inBufIdx_] = x;
#endif
        }

        // interpolate the most recent input samples
        // @param f in [0, 1]
        sample_t interpolate(phase_t f){
#ifdef RESAMPLER_INTERPOLATE_LINEAR
            return x_1_ + (x_ - x_1_) * f;
#else
            unsigned int i0, i1, i2, i3;
            i0 = (inBufIdx_ + 1) & IN_BUF_MASK;
            i1 = (inBufIdx_ + 2) & IN_BUF_MASK;
            i2 = (inBufIdx_ + 3) & IN_BUF_MASK;
            i3 = inBufIdx_;
            return static_cast<sample_t>(Interpolate::hermite<phase_t>(f, inBuf_[i0],
                                                                       inBuf_[i1],
                                                                       inBuf_[i2],
                                                                       inBuf_[i3]));
#endif
        }


        // write, upsampling
        // return frames written (>= 1)
        // assumptions: input has been pushed. rate_ > 1.0
        int writeUp(){
            phase_t p = phase_ + rate_;
            auto nf = static_cast<unsigned int>(p);
            // we can assume that n >= 1 here
            // we want to track fractional output phase for interpolation
            // this is normalized to the distance between input frames
            // so: the distance to the first output frame boundary:
            phase_t f = 1.0 - phase_;
            // normalized (divided by rate);
            f = f * phi_;
            // write the first output frame
            unsigned int i=0;
            outBuf_[i] = interpolate(f);
            while(i < nf) {
                // distance between output frames in this normalized space is 1/rate
                f += phi_;
                outBuf_[i] = interpolate(f);
                i++;
            }
            // store the remainder of the updated, un-normalized output phase
            phase_ = p - static_cast<phase_t>(nf);
            return nf;
        }


        // write, downsampling
        // return frames written (0 or 1)
        // assumptions: input has been pushed. rate_ <= 1.0
        int writeDown(){
            // number of frames will be 1 or 0.
            // as with upsampling inner loop,
            // we need to produce a fractional interpolation coefficient,
            // by "normalizing" to the output phase period
            phase_t p = phase_ + rate_;
            BOOST_ASSERT_MSG(p >= 0.0, "resampler encountered negative phase");
            auto nf = static_cast<unsigned int>(p);
            if (nf > 0) {
                phase_t f = 1.0 - phase_;
                f *= phi_;
                outBuf_[0] = interpolate(f);
                phase_ = p - static_cast<phase_t>(nf);
            } else {
                phase_ = p;
            }
            return nf;
        }

    };

}

#endif //SOFTCUTHEAD_RESAMPLER_H

