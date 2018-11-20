//
// Created by emb on 11/10/18.
//

#include <thread>

#include "Commands.h"
#include "FadeCurves.h"
#include "SoftCut.h"

using namespace softcut;

SoftCut::SoftCut() {
    init();
}

void SoftCut::init() {

    for (auto &v : scv) {
        v.setBuffer(buf, bufFrames);
    }

    // FIXME? wrong place for this probly
    FadeCurves::setPreShape(FadeCurves::Shape::LINEAR);
    FadeCurves::setRecShape(FadeCurves::Shape::RAISED);
    FadeCurves::setMinPreWindowFrames(0);
    FadeCurves::setMinRecDelayFrames(0);
    FadeCurves::setPreWindowRatio(1.f/8);
    FadeCurves::setRecDelayRatio(1.f/(8*16));
}

void SoftCut::processBlock(const float *in0, const float* in1, float *out0, float* out1, int numFrames) {
    (void)in1;
    Commands::handlePending(this);

    for(int fr=0; fr<numFrames; ++fr) {
        out0[fr] = 0;
        out1[fr] = 0;
    }
    for (int v=0; v<numVoices; ++v) {
        scv[v].processBlockMono(in0, outBus, numFrames);
        float amp0 = outAmp[v][0];
        float amp1 = outAmp[v][1];
        for(int fr=0; fr<numFrames; ++fr) {
            out0[fr] += outBus[fr] * amp0;
            out1[fr] += outBus[fr] * amp1;
        }
    }
}

void SoftCut::setSampleRate(unsigned int hz) {
    for (auto &v : scv) {
        v.setSampleRate(hz);
    }
}

void SoftCut::setRate(int voice, float rate) {
  scv[voice].setRate( rate);
}

void SoftCut::setLoopStart(int voice, float sec) {
  scv[voice].setLoopStart( sec);
}

void SoftCut::setLoopEnd(int voice, float sec) {
  scv[voice].setLoopEnd( sec);
}

void SoftCut::setLoopFlag(int voice, bool val) {
  scv[voice].setLoopFlag( val);
}

void SoftCut::setFadeTime(int voice, float sec) {
  scv[voice].setFadeTime( sec);
}

void SoftCut::setRecLevel(int voice, float amp) {
  scv[voice].setRecLevel( amp);
}

void SoftCut::setPreLevel(int voice, float amp) {
  scv[voice].setPreLevel( amp);
}

void SoftCut::setRecFlag(int voice, bool val) {
  scv[voice].setRecFlag( val);
}

void SoftCut::cutToPos(int voice, float sec) {
    scv[voice].cutToPos(sec);
}

void SoftCut::setFilterFc(int voice, float x) {
  scv[voice].setFilterFc(x);
}

void SoftCut::setFilterRq(int voice, float x) {
  scv[voice].setFilterRq(x);
}

void SoftCut::setFilterLp(int voice, float x) {
  scv[voice].setFilterLp(x);
}

void SoftCut::setFilterHp(int voice, float x) {
  scv[voice].setFilterHp(x);
}

void SoftCut::setFilterBp(int voice, float x) {
  scv[voice].setFilterBp(x);
}

void SoftCut::setFilterBr(int voice, float x) {
  scv[voice].setFilterBr(x);
}

void SoftCut::setFilterDry(int voice, float x) {
  scv[voice].setFilterDry(x);
}

void SoftCut::setFilterFcMod(int voice, float x) {
  scv[voice].setFilterFcMod( x);
}

void SoftCut::setAmpLeft(int voice, float x) {
    outAmp[voice][0] = x;
}

void SoftCut::setAmpRight(int voice, float x) {
    outAmp[voice][1] = x;
}

// NB fade curve recalculation happens on a separate thread
// the update function first fills a new buffer, then copies the final values
// this should mitigate glitches a little, but they can certainly still occur
// if fade curves are updated while loop recording is active
void SoftCut::setPreFadeWindow(float x) {
    auto t = std::thread([x] {
        FadeCurves::setPreWindowRatio(x);
    });
    t.detach();
}

void SoftCut::setRecFadeDelay(float x) {
    auto t = std::thread([x] {
        FadeCurves::setRecDelayRatio(x);
    });
    t.detach();
}


void SoftCut::setPreFadeShape(float x) {
    auto t = std::thread([x] {
        FadeCurves::setPreShape(static_cast<FadeCurves::Shape>(x));
    });
    t.detach();
}


void SoftCut::setRecFadeShape(float x) {
    auto t = std::thread([x] {
        FadeCurves::setRecShape(static_cast<FadeCurves::Shape>(x));
    });
    t.detach();
}

void SoftCut::printTestBuffers() {
    scv[0].printTestBuffers();
}

void SoftCut::setRecOffset(int i, float d) {
    scv[i].setRecOffset(d);
}

void SoftCut::setLevelSlewTime(int i, float d) {
    scv[i].setLevelSlewTime(d);
}

void SoftCut::setRateSlewTime(int i, float d) {
    scv[i].setRateSlewTime(d);
}
