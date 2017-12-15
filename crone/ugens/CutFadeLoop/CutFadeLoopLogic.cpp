//
// Created by ezra on 12/6/17.
//

// TODO: looping/1shot behavior flag

#include <cmath>
#include "CutFadeLoopLogic.h"

#include "interp.h"

/// wtf...
#ifndef nullptr
#define nullptr ((void*)0)
#endif

static int wrap(int val, int bound) {
    if(val >= bound) { return val - bound; }
    if(val < 0) { return val + bound; }
    return val;
}


CutFadeLoopLogic::CutFadeLoopLogic() {
    this->init();
}

void CutFadeLoopLogic::init() {
    sr = 44100.f;
    start = 0.f;
    end = 0.f;
    phase[0] = 0.f;
    phase[1] = 0.f;
    fade[0] = 0.f;
    fade[1] = 0.f;
    state[0] = INACTIVE;
    state[1] = INACTIVE;
    active = 0;
    phaseInc = 0.f;
    setFadeTime(0.1f);
    buf = (const float*) nullptr;
    bufFrames = 0;
    trig[0] = 0.f;
    trig[1] = 0.f;
    fadeMode = FADE_EQ;
}

void CutFadeLoopLogic::nextSample(float *outAudio, float *outPhase, float *outTrig) {

    if(buf == nullptr) {
        return;
    }

    updatePhase(0);
    updatePhase(1);
    updateFade(0);
    updateFade(1);

    if(outPhase != nullptr) { *outPhase = phase[active] / sr; }

//    *outAudio = peek(phase[0]) * fade[0] + peek(phase[1]) * fade[1];
    *outAudio = mixFade(peek(phase[0]), peek(phase[1]), fade[0], fade[1]);
    *outTrig = trig[0] + trig[1];
}


void CutFadeLoopLogic::setRate(float x)
{
    phaseInc = x;

}

void CutFadeLoopLogic::setLoopStartSeconds(float x)
{
    start = x * sr;
}

void CutFadeLoopLogic::setLoopEndSeconds(float x)
{
    end = x * sr;
}

void CutFadeLoopLogic::updatePhase(int id)
{
    float p;
    trig[id] = 0.f;
    switch(state[id]) {
        case FADEIN:
        case FADEOUT:
        case ACTIVE:
            p = phase[id] + phaseInc;
            if(id == active) {
                if (phaseInc > 0.f) {
                    if (p > end) {
                        if (loopFlag) {
                            // cutToPos(start + (p-end)); // preserve phase overshoot?
                            cutToPhase(start);
                            trig[id] = 1.f;
                        } else {
                            state[id] = FADEOUT;
                        }
                    }
                } else { // negative rate
                    if (p < start) {
                        if(loopFlag) {
                            // cutToPos(end + (p - start));
                            cutToPhase(end);
                            trig[id] = 1.f;
                        } else {
                            state[id] = FADEOUT;
                        }
                    }
                } // rate sign check
            } // /active check
            phase[id] = p;
            break;
        case INACTIVE:
        default:
            ;; // nothing to do
    }
}

void CutFadeLoopLogic::cutToPhase(float pos) {
    int newActive = active == 0 ? 1 : 0;
    if(state[active] != INACTIVE) {
        state[active] = FADEOUT;
    }
    state[newActive] = FADEIN;
    phase[newActive] = pos;
    active = newActive;
}

void CutFadeLoopLogic::updateFade(int id) {
    switch(state[id]) {
        case FADEIN:
            fade[id] += fadeInc;
            if (fade[id] > 1.f) {
                fade[id] = 1.f;
                doneFadeIn(id);
            }
            break;
        case FADEOUT:
            fade[id] -= fadeInc;
            if (fade[id] < 0.f) {
                fade[id] = 0.f;
                doneFadeOut(id);
            }
            break;
        case ACTIVE:
        case INACTIVE:
        default:;; // nothing to do
    }
}

void CutFadeLoopLogic::setFadeTime(float secs) {
    fadeInc = (float) 1.0 / (secs * sr);
}

void CutFadeLoopLogic::doneFadeIn(int id) {
    state[id] = ACTIVE;
}

void CutFadeLoopLogic::doneFadeOut(int id) {
    state[id] = INACTIVE;
}

float CutFadeLoopLogic::peek(float phase) {

    int phase1 = (int)phase;
    int phase0 = phase1 - 1;
    int phase2 = phase1 + 1;
    int phase3 = phase1 + 2;

    float y0 = buf[wrap(phase0, bufFrames)];
    float y1 = buf[wrap(phase1, bufFrames)];
    float y2 = buf[wrap(phase2, bufFrames)];
    float y3 = buf[wrap(phase3, bufFrames)];

    float x = phase - (float)phase1;
    return cubicinterp(x, y0, y1, y2, y3);
}

void CutFadeLoopLogic::setBuffer(const float *b, uint32_t bf) {
    buf = b;
    bufFrames = bf;
}

void CutFadeLoopLogic::setLoopFlag(bool val) {
    loopFlag = val;
}

void CutFadeLoopLogic::cutToStart() {
    cutToPhase(start);
}

void CutFadeLoopLogic::setSampleRate(float sr_) {
    sr = sr_;
}

float CutFadeLoopLogic::mixFade(float x, float y, float a, float b) {
    switch (fadeMode) {

        case FADE_EQ:
            return x * cosf(a * (float)M_PI_2) + y * cosf(b * (float)M_PI_2);
            break;
        case FADE_LIN:
        case FADE_EXP: // FIXME! use a LUT for exp
        default:
            return x * a + y * b;
    }
}
