//
// Created by ezra on 12/6/17.
//

// TODO: looping/1shot behavior flag

#include <cmath>
#include <limits>


#include "interp.h"
#include "Resampler.h"

#include "SoftCutHeadLogic.h"


/// wtf...
#ifndef nullptr
#define nullptr ((void*)0)
#endif

using namespace softcuthead;

static int wrap(int val, int bound) {
    if(val >= bound) { return val - bound; }
    if(val < 0) { return val + bound; }
    return val;
}


SoftCutHeadLogic::SoftCutHeadLogic() {
    this->init();
}

void SoftCutHeadLogic::init() {
    sr = 44100.f;
    start = 0.f;
    end = 0.f;
    active = 0;
    phaseInc = 0.f;
    setFadeTime(0.1f);
    fadeMode = FADE_EQ;
    recRun = false;
}

void SoftCutHeadLogic::nextSample(float in, float *outPhase, float *outTrig, float *outAudio) {

    if(buf == nullptr) {
        return;
    }

    *outAudio = mixFade(head[0].peek(), head[1].peek(), head[0].fade(), head[1].fade());
    *outTrig = head[0].trig() + head[1].trig();
    if(outPhase != nullptr) { *outPhase = static_cast<float>(head[active].phase()); }

    if(recRun) {
        head[0].poke(in, pre, rec, fadePre, fadeRec);
        head[1].poke(in, pre, rec, fadePre, fadeRec);
    }

    Action act0 = head[0].updatePhase(phaseInc, start, end, loopFlag);
    takeAction(act0, 0);
    Action act1 = head[1].updatePhase(phaseInc, start, end, loopFlag);
    takeAction(act1, 1);

    head[0].updateFade(fadeInc);
    head[1].updateFade(fadeInc);
}


void SoftCutHeadLogic::setRate(float x)
{
    phaseInc = x;
}

void SoftCutHeadLogic::setLoopStartSeconds(float x)
{
    start = x * sr;
}

void SoftCutHeadLogic::setLoopEndSeconds(float x)
{
    end = x * sr;
}


void SoftCutHeadLogic::takeAction(Action act, int id)
{
    switch (act) {
        case Action::LOOP_POS:
            cutToPhase(start);
            break;
        case Action::LOOP_NEG:
            cutToPhase(end);
            break;
        case Action::STOP:
        case Action::NONE:
        default: ;;
    }

}

void SoftCutHeadLogic::cutToPhase(float pos) {
    State s = head[active].state();


    if(s == State::FADEIN || s == State::FADEOUT) { return; }
    int newActive = active == 0 ? 1 : 0;
    if(s != State::INACTIVE) {
        head[active].setState(State::FADEOUT);
    }
    head[newActive].setState(State::FADEIN);
    head[newActive].setPhase(pos);
    head[active].active_ = false;
    head[newActive].active_ = true;
    active = newActive;
}

void SoftCutHeadLogic::setFadeTime(float secs) {
    fadeInc = (float) 1.0 / (secs * sr);
}


void SoftCutHeadLogic::setBuffer(float *b, uint32_t bf) {
    buf = b;
    head[0].setBuffer(b, bf);
    head[1].setBuffer(b, bf);
}

void SoftCutHeadLogic::setLoopFlag(bool val) {
    loopFlag = val;
}

void SoftCutHeadLogic::setSampleRate(float sr_) {
    sr = sr_;
}

float SoftCutHeadLogic::mixFade(float x, float y, float a, float b) {
    if(fadeMode == FADE_EQ) {
        return x * sinf(a * (float) M_PI_2) + y * sinf(b * (float) M_PI_2);
    } else {
        return (x * a) + (y * b);
    }
}

void SoftCutHeadLogic::setRec(float x) {
    rec = x;
}


void SoftCutHeadLogic::setPre(float x) {
    pre= x;
}

void SoftCutHeadLogic::setFadePre(float x) {
    fadePre = x;
}

void SoftCutHeadLogic::setFadeRec(float x) {
    fadeRec = x;
}

void SoftCutHeadLogic::setRecRun(bool val) {
    recRun = val;
}

void SoftCutHeadLogic::setRecOffset(float x) {
    recPhaseOffset = x;
}
