//
// Created by ezra on 12/6/17.
//

#include "CutFadeLoopLogic.h"

// enumerate read-head states
static enum { ACTIVE, INACTIVE, FADEIN, FADEOUT };


CutFadeLoopLogic::CutFadeLoopLogic(float sr_, float* buf_, int bufSize_) :
        sr(sr_), buf(buf_), bufSize(bufSize_)
{
    start = 0;
    end = bufSize;

    phase[0] = phase[1] = 0.f;
    amp[0] = amp[1] = 0.f;
    state[0] = state[1] = INACTIVE;
    active = 0;
}

void CutFadeLoopLogic::nextSample(float *outAudio, float *outPhase) {

}

void CutFadeLoopLogic::setRate(float x)
{
    phi = x;
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
    float p = phase[id];
    p += phi;
    if(phi > 0.f) {
        if (p >= end) {
            p = start + (end-phi);
            cutToPos(p);
        }
    } else {

    }
    phase[id] = p; // store updated phase
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
