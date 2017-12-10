//
// Created by ezra on 12/6/17.
//

// TODO: looping/1shot behavior flag

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
    isWrapping = false;
    wasWrapping = false;
}

void CutFadeLoopLogic::nextSample(float *outAudio, float *outPhase, float *outTrig) {

    if(buf == nullptr) {
        return;
    }

    updatePhase(0);
    updatePhase(1);
    updateFade(0);
    updateFade(1);

    // FIXME: connect phase output in ugen wrapper
    if(outPhase != nullptr) { *outPhase = phase[active] / sr; }

    // TODO: linear fade for now. add cosine, exp via LUT
     *outAudio = peek(phase[0]) * fade[0] + peek(phase[1]) * fade[1];

    if(isWrapping && !wasWrapping) {
        *outTrig = 1.f;
    } else {
        *outTrig = 0.f;
    }
    wasWrapping = isWrapping;
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
    switch(state[id]) {
        case FADEIN:
        case FADEOUT:
        case ACTIVE:
            p = phase[id] + phaseInc;
            if(id == active) {
                if (phaseInc > 0.f) {
                    if (p > end) {
                        if (loopFlag) {
                            // cutToPos(start + (p-end));
                            cutToPhase(start);
                            isWrapping = true;
                        } else {
                            state[id] = FADEOUT;
                        }
                    } else {
                        isWrapping = false;
                    }
                } else { // negative rate
                    if (p < start) {
                        if(loopFlag) {
                            // cutToPos(end + (p - start));
                            cutToPhase(end);
                            isWrapping = true;
                        } else {
                            state[id] = FADEOUT;
                        }
                    } else {
                        isWrapping = false;
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
