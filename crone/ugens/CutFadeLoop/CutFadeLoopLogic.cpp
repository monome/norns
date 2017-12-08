//
// Created by ezra on 12/6/17.
//

// TODO: looping/1shot behavior flag

#include "CutFadeLoopLogic.h"

CutFadeLoopLogic::CutFadeLoopLogic()
{
    start = 0;
    end = 0;
    phase[0] = 0.f;
    phase[1] = 0.f;
    fade[0] = 0.f;
    fade[1] = 0.f;
    state[0] = INACTIVE;
    state[1] = INACTIVE;
    active = 0;
    setFadeTime(0.1);
}

void CutFadeLoopLogic::nextSample(float *outAudio, float *outPhase) {
    updatePhase(0);
    updatePhase(1);
    updateFade(0);
    updateFade(1);
    if(outPhase != nullptr) { *outPhase = phase[active]; }
    // TODO: linear fade for now. add cosine, exp via LUT
     *outAudio = peek(phase[0]) * fade[0] + peek(phase[1]) * fade[1];
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
            p = phase[id];
            p += phaseInc;
            if(id == active) {
                if (phaseInc > 0.f) {
                    if (p > end) {
                        if(loopFlag) {
                            // cutToPos(start + (p-end));
                            cutToPos(start);
                            // TODO: add trigger output on loop?
                        } else {
                            state[id] = FADEOUT;
                        }
                    }

                } else { // negative rate
                    if (p < start) {
                        if(loopFlag) {
                            // cutToPos(end + (p - start));
                            cutToPos(end);
                            // TODO: add trigger output on loop?
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

void CutFadeLoopLogic::cutToPos(float pos) {
    int newActive = active ^1;
    state[active] = FADEOUT;
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
    // TODO: ahahaha, not interpolating r/n
    return buf[((int)phase) % bufFrames];
}

void CutFadeLoopLogic::setBuffer(const float *b, uint32_t bf) {
    buf = b;
    bufFrames = bf;
}

void CutFadeLoopLogic::setLoopFlag(bool val) {
    loopFlag = val;
}

void CutFadeLoopLogic::resetPos() {
    cutToPos(start);
}

void CutFadeLoopLogic::setSampleRate(float sr_) {
    sr = sr_;
}
