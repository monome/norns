//
// Created by ezra on 12/6/17.
//

// TODO: looping/1shot behavior flag

#include "CutFadeLoopLogic.h"

// enumerate read-head states
static enum { ACTIVE, INACTIVE, FADEIN, FADEOUT };


CutFadeLoopLogic::CutFadeLoopLogic(float sr_, float* buf_, int bufSize_) :
        sr(sr_), buf(buf_), bufSize(bufSize_)
{
    start = 0;
    end = bufSize;
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

void CutFadeLoopLogic::setLoopEndSeconds(float x) {
    end = x * sr;
}

void CutFadeLoopLogic::updatePhase(int id)
{
    switch(state[id]) {
        case FADEIN:
        case FADEOUT:
        case ACTIVE:
            float p = phase[id];
            p += phaseInc;
            if (phaseInc > 0.f) {
                if (p > end) {
                    p = start + (phaseInc - end);
                    cutToPos(p); // also updates phase
                    // TODO: add trigger output on loop
                    return;
                }
            } else {
                if (phaseInc < start) {
                    p = end + (phaseInc - start);
                    cutToPos(p); // also updates phase
                    // TODO: add trigger output on loop
                    return;
                }
            }
            phase[id] = p; // no cut was needed; store updated phase
            break;

        case INACTIVE: default: ;; // nothing to do
    }
}

void CutFadeLoopLogic::setPosSeconds(float x) {
    cutToPos(x * sr);
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
                doneFadeIn(id);
            }
            break;
        case FADEOUT:
            fade[id] -= fadeInc;
            if (fade[id] > 1.f) {
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
