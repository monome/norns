//
// Created by ezra on 12/6/17.
//

#include <cmath>
#include "CutFadeVoiceLogic.h"

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

CutFadeVoiceLogic::CutFadeVoiceLogic() {
    this->init();
}

void CutFadeVoiceLogic::init() {
    sr = 44100.f;
    start = 0.f;
    end = 0.f;
    phase_rd[0] = 0.f;
    phase_rd[1] = 0.f;
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
    // fadeMode = FADE_LIN;
    fadeMode = FADE_EQ;
}

void CutFadeVoiceLogic::nextSample(float* in, float* outPhase, float *outTrig, float* outAudio) {

    if(buf == nullptr) {
        return;
    }

    updatePhase(0);
    updatePhase(1);
    updateFade(0);
    updateFade(1);

    if(outPhase != nullptr) { *outPhase = phase_rd[active] / sr; }

    *outAudio = mixFade(peek(phase_rd[0]), peek(phase_rd[1]), fade[0], fade[1]);
    *outTrig = trig[0] + trig[1];
}


void CutFadeVoiceLogic::setRate(float x)
{
    phaseInc = x;

}

void CutFadeVoiceLogic::setLoopStartSeconds(float x)
{
    start = x * sr;
}

void CutFadeVoiceLogic::setLoopEndSeconds(float x)
{
    end = x * sr;
}

void CutFadeVoiceLogic::updatePhase(int id)
{
    float p;
    trig[id] = 0.f;
    switch(state[id]) {
        case FADEIN:
        case FADEOUT:
        case ACTIVE:
            if(id == active) {
                p = phase_rd[id] + phaseInc;
                applyPhase(id, p);
                phase_rd[id] = p;
            } // /active check
            break;
        case INACTIVE:
        default:
            ;; // nothing to do
    }
}

void CutFadeVoiceLogic::applyPhase(int id, float p) {
    if (phaseInc > 0.f) {
        if (p > end) {
            if (loopFlag) {
                cutToPhase(start + (p - end)); // preserve phase overshoot
                //cutToPhase(start);
                trig[id] = 1.f;
            } else {
                state[id] = FADEOUT;
            }
        }
    } else { // negative rate
        if (p < start) {
            if(loopFlag) {
                cutToPhase(end + (p - start)); // preserve phase overshoot
                //cutToPhase(end);
                trig[id] = 1.f;
            } else {
                state[id] = FADEOUT;
            }
        }
    } // rate sign check
}

void CutFadeVoiceLogic::cutToPhase(float pos) {
    int newActive = active == 0 ? 1 : 0;
    if(state[active] != INACTIVE) {
        state[active] = FADEOUT;
    }
    state[newActive] = FADEIN;
    phase_rd[newActive] = pos;
    active = newActive;
}

void CutFadeVoiceLogic::updateFade(int id) {
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

void CutFadeVoiceLogic::setFadeTime(float secs) {
    fadeInc = (float) 1.0 / (secs * sr);
}

void CutFadeVoiceLogic::doneFadeIn(int id) {
    state[id] = ACTIVE;
}

void CutFadeVoiceLogic::doneFadeOut(int id) {
    state[id] = INACTIVE;
}


float CutFadeVoiceLogic::peek(float phase) {
    return peek4(float phase);
}

float CutFadeVoiceLogic::peek4(float phase) {

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


void CutFadeVoiceLogic::poke(float phase, float x) {
    poke2(phase, x);
}

void CutFadeVoiceLogic::poke2(float phase, float x) {

    int phase1 = (int)phase;
    int phase0 = phase1 - 1;
    int phase2 = phase1 + 1;
    int phase3 = phase1 + 2;

    float y0 = buf[wrap(phase0, bufFrames)];
    float y1 = buf[wrap(phase1, bufFrames)];
    float y2 = buf[wrap(phase2, bufFrames)];
    float y3 = buf[wrap(phase3, bufFrames)];

    float x = phase - (float)phase1;
    //return cubicinterp(x, y0, y1, y2, y3);
}

void CutFadeVoiceLogic::setBuffer(const float *b, uint32_t bf) {
    buf = b;
    bufFrames = bf;
}

void CutFadeVoiceLogic::setLoopFlag(bool val) {
    loopFlag = val;
}

void CutFadeVoiceLogic::cutToStart() {
    cutToPhase(start);
}

void CutFadeVoiceLogic::setSampleRate(float sr_) {
    sr = sr_;
}

float CutFadeVoiceLogic::mixFade(float x, float y, float a, float b) {
    if(fadeMode == FADE_EQ) {
        return x * sinf(a * (float) M_PI_2) + y * sinf(b * (float) M_PI_2);
    } else { // FIXME: add LUT for EXP
        return (x * a) + (y * b);
    }
}
