//
// Created by ezra on 12/6/17.
//
#include <cmath>
#include <limits>


#include "Interpolate.h"
#include "Resampler.h"

#include "SoftCutHead.h"

using namespace softcut;
using namespace std;

SoftCutHead::SoftCutHead() {
    this->init();
}

void SoftCutHead::init() {
    sr = 44100.f;
    start = 0.f;
    end = 0.f;
    active = 0;
    rate = 1.f;
    setFadeTime(0.1f);
    recFlag = false;
    testBuf.init();
}

void SoftCutHead::processSample(sample_t in, float *outPhase, float *outTrig, sample_t *outAudio) {

#if 1 // testing...
    testBuf.update(static_cast<float>(head[0].phase_), head[0].wrIdx_, head[0].fade_, head[0].state_, head[0].preFade, head[0].recFade);
#endif

    *outAudio = mixFade(head[0].peek(), head[1].peek(), head[0].fade(), head[1].fade());
    *outTrig = head[0].trig() + head[1].trig();

    if(outPhase != nullptr) {
        *outPhase = static_cast<float>(head[active].phase());
    }

    int numFades = (head[0].state_ == FADEIN || head[0].state_ == FADEOUT)
            + (head[1].state_ == FADEIN || head[1].state_ == FADEOUT);

    BOOST_ASSERT_MSG(!(head[0].state_ == ACTIVE && head[1].state_ == ACTIVE), "multiple active heads");


    // FIXME: should probably continue to push input to resampler when not recording
    if(recFlag) {
        head[0].poke(in, pre, rec, numFades);
        head[1].poke(in, pre, rec, numFades);
    }
    takeAction(head[0].updatePhase(start, end, loopFlag));
    takeAction(head[1].updatePhase(start, end, loopFlag));

    head[0].updateFade(fadeInc);
    head[1].updateFade(fadeInc);

}

void SoftCutHead::setRate(rate_t x)
{
    rate = x;
    calcFadeInc();
    head[0].setRate(x);
    head[1].setRate(x);
}

void SoftCutHead::setLoopStartSeconds(float x)
{
    start = x * sr;
}

void SoftCutHead::setLoopEndSeconds(float x)
{
    end = x * sr;
}

void SoftCutHead::takeAction(Action act)
{
    switch (act) {
        case Action::LOOP_POS:
            std::cerr << "looping: go to start" << std::endl;
            cutToPhase(start);
            break;
        case Action::LOOP_NEG:
            std::cerr << "looping: go to end" << std::endl;
            cutToPhase(end);
            break;
        case Action::STOP:
        case Action::NONE:
        default: ;;
    }
}

void SoftCutHead::cutToPhase(phase_t pos) {
    State s = head[active].state();

    // ignore if we are already in a crossfade
    if(s == State::FADEIN || s == State::FADEOUT) {
        // std::cout << "skipping phase change due to ongoing xfade" << std::endl;
        return;
    }

    // activate the inactive head
    int newActive = active ^ 1;
    if(s != State::INACTIVE) {
        head[active].setState(State::FADEOUT);
    }

    head[newActive].setState(State::FADEIN);
    head[newActive].setPhase(pos);

    head[active].active_ = false;
    head[newActive].active_ = true;
    active = newActive;
    std::cerr << "active: " << active << std::endl;
}

void SoftCutHead::setFadeTime(float secs) {
    fadeTime = secs;
    calcFadeInc();
}
void SoftCutHead::calcFadeInc() {
    fadeInc = (float) fabs(rate) / std::max(1.f, (fadeTime * sr));
    fadeInc = std::max(0.f, std::min(fadeInc, 1.f));
    // printf("fade time = %f; rate = %f; inc = %f\n", fadeTime, rate, fadeInc);
}

void SoftCutHead::setBuffer(float *b, uint32_t bf) {
    buf = b;
    head[0].setBuffer(b, bf);
    head[1].setBuffer(b, bf);
}

void SoftCutHead::setLoopFlag(bool val) {
    loopFlag = val;
}

void SoftCutHead::setSampleRate(float sr_) {
    sr = sr_;
    head[0].setSampleRate(sr);
    head[1].setSampleRate(sr);
}

sample_t SoftCutHead::mixFade(sample_t x, sample_t y, float a, float b) {
#if 1
        return x * sinf(a * (float)M_PI_2) + y * sinf(b * (float) M_PI_2);
#else
        return (x * a) + (y * b);
#endif
}

void SoftCutHead::setRec(float x) {
    rec = x;
}

void SoftCutHead::setPre(float x) {
    pre= x;
}

void SoftCutHead::setRecRun(bool val) {
    recFlag = val;
}

phase_t SoftCutHead::getActivePhase() {
  return head[active].phase();
}

float SoftCutHead::getTrig() {
  return head[0].trig()+ head[1].trig();
}

void SoftCutHead::resetTrig() {
  head[0].setTrig(0);
  head[1].setTrig(0);
}

void SoftCutHead::cutToPos(float seconds) {
    cutToPhase(seconds * sr);
}

rate_t SoftCutHead::getRate() {
    return rate;
}

void SoftCutHead::printTestBuffers() {
    testBuf.print();
}

void SoftCutHead::setRecOffset(float d) {
    head[0].setRecOffset(d);
    head[1].setRecOffset(d);
}
