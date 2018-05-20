//
// Created by ezra on 4/21/18.
//

#include "interp.h"
#include "SubHead.h"

using namespace softcuthead;


SubHead::SubHead(): resamp_(ringBuf, RING_BUF_SIZE) {
    this->init();
}

void SubHead::init() {
    phase_ = 0;
    fade_ = 0;
    trig_ = 0;
    state_ = INACTIVE;
    memset(ringBuf, 0, sizeof(ringBuf));
    resamp_.setPhase(0);
}

Action SubHead::updatePhase(double start, double end, bool loop) {
    Action res = NONE;
    trig_ = 0.f;
    double p;
    switch(state_) {
        case FADEIN:
        case FADEOUT:
        case ACTIVE:
            p = phase_ + rate_;
            if(active_) {
                if (rate_ > 0.f) {
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

    int inc = rate_ > 0.f ? 1 : -1;
    int nframes = resamp_.processFrame(in);

    idx = static_cast<int>(phase_);
    for(int i=0; i<nframes; ++i) {        
        y = resamp_.buffer()[wrap(frame + i, RING_BUF_SIZE)];
        buf_[idx] *= preFade;
        buf_[idx] += y * recFade;
	idx = wrap(static_cast<int>(idx + inc), bufFrames_);
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

    double y0 = buf_[wrap(phase0, bufFrames_)];
    double y1 = buf_[wrap(phase1, bufFrames_)];
    double y2 = buf_[wrap(phase2, bufFrames_)];
    double y3 = buf_[wrap(phase3, bufFrames_)];

    double x = phase_ - (double)phase1;
    return static_cast<float>(cubicinterp(x, y0, y1, y2, y3));
}

