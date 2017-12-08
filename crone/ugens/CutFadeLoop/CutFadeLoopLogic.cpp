//
// Created by ezra on 12/6/17.
//

// TODO: looping/1shot behavior flag

#include "CutFadeLoopLogic.h"

CutFadeLoopLogic::CutFadeLoopLogic(float sr_) : sr(sr_)
{
    start = 0;
    //end = bufSize;
    end = 0;
    phase[0] = phase[1] = 0.f;
    fade[0] = fade[1] = 0.f;
    state[0] = state[1] = INACTIVE;
    active = 0;
    setFadeTime(0.1);
}

void CutFadeLoopLogic::nextSample(float *outAudio, float *outPhase) {
    updatePhase(0);
    updatePhase(1);
    updateFade(0);
    updateFade(1);
    *outPhase = phase[active];
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
            if (phaseInc > 0.f) {
                if (p > end) {
                    if(loopFlag) {
                        cutToPos(start + (p-end));
                        // TODO: add trigger output on loop?
                    } else {
                        state[id] = FADEOUT;
                    }
                }

            } else { // negative rate
                if (phaseInc < start) {
                    if(loopFlag) {
                        //p = end + (phaseInc - start);
                        cutToPos(end + (p - start));
                        // TODO: add trigger output on loop?
                    } else {
                        state[id] = FADEOUT;
                    }
                }
            }
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
            if (fade[id] >= 1.f) {
                fade[id] = 1.f;
                doneFadeIn(id);
            }
            break;
        case FADEOUT:
            fade[id] -= fadeInc;
            if (fade[id] <= 0.f) {
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
    return buf[((int)phase) % bufSize];
}

void CutFadeLoopLogic::setBuffer(float *b, int size) {
    buf = b;
    bufSize = size;
}

void CutFadeLoopLogic::setLoopFlag(bool val) {
    loopFlag = val;
}

void CutFadeLoopLogic::resetPos() {
    cutToPos(start);
}
