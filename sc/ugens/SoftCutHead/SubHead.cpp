//
// Created by ezra on 4/21/18.
//

#include "interp.h"
#include "SubHead.h"

using namespace softcuthead;

static int wrap(int val, int bound) {
    if(val >= bound) { return val - bound; }
    if(val < 0) { return val + bound; }
    return val;
}


SubHead::SubHead(): resamp_(ringBuf, RING_BUF_SIZE){
    this->init();
}

void SubHead::init() {
    phase_ = 0;
    fade_ = 0;
    trig_ = 0;
    state_ = INACTIVE;
    memset(ringBuf, 0, sizeof(ringBuf) * sizeof(float));
}

Action SubHead::updatePhase(double inc, double start, double end, bool loop) {
    Action res = NONE;
    trig_ = 0.f;
    double p;
    switch(state_) {
        case FADEIN:
        case FADEOUT:
        case ACTIVE:
            p = phase_ + inc;
            if(active_) {
                if (inc > 0.f) {
                    if (p > end || p < start) {
                        if (loop) {
                            trig_ = 1.f;
                            res = LOOP_POS;
                        } else {
                            state_ = FADEOUT;
                            res = STOP;
                        }
                    }
                } else { // negative rate
                    if (p > end || p < start) {
                        if(loop) {
                            trig_ = 1.f;
                            res = LOOP_NEG;
                        } else {
                            state_ = FADEOUT;
                            res = STOP;
                        }
                    }
                } // rate sign check
            } // /active check
            phase_ = p;
            break;
        case INACTIVE:
        default:
            ;; // nothing to do
    }
    return res;
}

void SubHead::updateFade(double inc) {
    switch(state_) {
        case FADEIN:
            fade_ += inc;
            if (fade_ > 1.f) {
                fade_ = 1.f;
                state_ = ACTIVE;
            }
            break;
        case FADEOUT:
            fade_ -= inc;
            if (fade_ < 0.f) {
                fade_ = 0.f;
                state_ = INACTIVE;
            }
            break;
        case ACTIVE:
        case INACTIVE:
        default:;; // nothing to do
    }
}

void SubHead::poke(float in, float pre, float rec, float fadePre, float fadeRec) {
    if (fade_ < std::numeric_limits<float>::epsilon()) { return; }
    if (rec < std::numeric_limits<float>::epsilon()) { return; }

    if(state_ == INACTIVE) {
        return;
    }
    float fadeInv = 1.f - fade_;
    float preFade = pre * (1.f - fadePre) + fadePre * std::fmax(pre, (pre * fadeInv));
    float recFade = rec * (1.f - fadeRec) + fadeRec * (rec * fade_);

    int idx; // write index
    float y; // write value
    int frame = resamp_.frame();

    // FIXME: set resampler rate
    // FIXME: allow for negative rates (see VariWrite for method)

    int nframes = resamp_.processFrame(in);
    for(int i=0; i<nframes; ++i) {
        idx = wrap(static_cast<int>(phase_ + i), bufFrames_);
        y = resamp_.getBuffer()[wrap(frame + i, RING_BUF_SIZE)];
        buf_[idx] *= preFade;
        buf_[idx] += y * recFade;
    }
}


float SubHead::peek() {
    return peek4(phase_);
}


float SubHead::peek4(double phase) {
    int phase1 = static_cast<int>(phase_);
    int phase0 = phase1 - 1;
    int phase2 = phase1 + 1;
    int phase3 = phase1 + 2;

    const float* buf = resamp_.getBuffer();
    const int bufFrames = resamp_.getBufferFrames();

    double y0 = buf[wrap(phase0, bufFrames)];
    double y1 = buf[wrap(phase1, bufFrames)];
    double y2 = buf[wrap(phase2, bufFrames)];
    double y3 = buf[wrap(phase3, bufFrames)];

    double x = phase_ - (double)phase1;
    return static_cast<float>(cubicinterp(x, y0, y1, y2, y3));
}