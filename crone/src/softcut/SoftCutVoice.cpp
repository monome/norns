//
// Created by ezra on 11/3/18.
//

#include "SoftCutVoice.h"
#include <functional>

using namespace softcut;

SoftCutVoice::SoftCutVoice() :
rateRamp(48000, 0.1),
preRamp(48000, 0.1),
recRamp(48000, 0.1)
{
    fcBase = 16000;
    sch.init();
    svf.setLpMix(1.0);
    svf.setHpMix(0.0);
    svf.setBpMix(0.0);
    svf.setBrMix(0.0);
    svf.setRq(20.0);
    svf.setFc(fcBase);
    svfDryLevel = 1.0;
    recFlag = false;
    playFlag = true;
}

void SoftCutVoice:: processBlockMono(const float *in, float *out, int numFrames) {
    std::function<void(sample_t, sample_t*)> sampleFunc;
    if(playFlag) {
        if(recFlag) {
            sampleFunc = [this](float in, float* out) {
                this->sch.processSample(in, out);
            };
        } else {
            sampleFunc = [this](float in, float* out) {
                this->sch.processSampleNoWrite(in, out);
            };
        }
    } else {
        if(recFlag) {
            sampleFunc = [this](float in, float* out) {
                this->sch.processSampleNoRead(in, out);
            };
        } else {
            // FIXME? do nothing, i guess?
            sampleFunc = [](float in, float* out) {
                (void)in;
                (void)out;
            };
        }
    }

    float x;
    for(int i=0; i<numFrames; ++i) {
        x = svf.getNextSample(in[i]) + in[i]*svfDryLevel;
        sch.setRate(rateRamp.update());
        sch.setPre(preRamp.update());
        sch.setRec(recRamp.update());
        sampleFunc(x, &(out[i]));
        updateQuantPhase();
    }
}

void SoftCutVoice::setSampleRate(float hz) {
    sampleRate = hz;
    rateRamp.setSampleRate(hz);
    preRamp.setSampleRate(hz);
    recRamp.setSampleRate(hz);
    sch.setSampleRate(sampleRate);
    svf.setSampleRate(hz);
}

void SoftCutVoice::setRate(float rate) {
    rateRamp.setTarget(rate);
    updateFilterFc();
}

void SoftCutVoice::setLoopStart(float sec) {
    sch.setLoopStartSeconds(sec);
}

void SoftCutVoice::setLoopEnd(float sec) {
    sch.setLoopEndSeconds(sec);
}

void SoftCutVoice::setFadeTime(float sec) {
    sch.setFadeTime(sec);
}

void SoftCutVoice::cutToPos(float sec) {
    sch.cutToPos(sec);
}

void SoftCutVoice::setRecLevel(float amp) {
    recRamp.setTarget(amp);
}

void SoftCutVoice::setPreLevel(float amp) {
    preRamp.setTarget(amp);
}

void SoftCutVoice::setRecFlag(bool val) {
    recFlag = val;
}


void SoftCutVoice::setPlayFlag(bool val) {
    playFlag = val;
}

void SoftCutVoice::setLoopFlag(bool val) {
    sch.setLoopFlag(val);
}

void SoftCutVoice::setFilterFc(float x) {
    fcBase = x;
    updateFilterFc();
}

void SoftCutVoice::setFilterRq(float x) {
    svf.setRq(x);
}

void SoftCutVoice::setFilterLp(float x) {
    svf.setLpMix(x);
}

void SoftCutVoice::setFilterHp(float x) {
    svf.setHpMix(x);
}

void SoftCutVoice::setFilterBp(float x) {
    svf.setBpMix(x);
}

void SoftCutVoice::setFilterBr(float x) {
    svf.setBrMix(x);
}

void SoftCutVoice::setFilterDry(float x) {
    svfDryLevel = x;
}

void SoftCutVoice::setFilterFcMod(float x) {
    fcMod = x;
}

void SoftCutVoice::updateFilterFc() {
    float fc = std::min(fcBase, fcBase * std::fabs(static_cast<float>(sch.getRate())));
    // std::cout << fc << std::endl;
    svf.setFc(fc*fcMod + (1.f-fcMod )*svf.getFc());
}

void SoftCutVoice::setBuffer(float *b, unsigned int nf) {
    buf = b;
    bufFrames = nf;
    sch.setBuffer(buf, bufFrames);
}

void SoftCutVoice::setRecOffset(float d) {
    sch.setRecOffsetSamples(static_cast<int>(d * sampleRate));
}

void SoftCutVoice::setLevelSlewTime(float d) {
    recRamp.setTime(d);
    preRamp.setTime(d);
}

void SoftCutVoice::setRateSlewTime(float d) {
    rateRamp.setTime(d);
}

void SoftCutVoice::setPhaseQuant(float x) {
    phaseQuant = x;
}


phase_t SoftCutVoice::getQuantPhase() {
    return quantPhase;
}

void SoftCutVoice::updateQuantPhase() {
    if (phaseQuant == 0) {
        quantPhase = sch.getActivePhase() / sampleRate;
    } else {
        quantPhase = std::floor(sch.getActivePhase() / (sampleRate *phaseQuant)) * phaseQuant;
    }
}

bool SoftCutVoice::getPlayFlag() {
    return playFlag;
}

bool SoftCutVoice::getRecFlag() {
    return recFlag;
}

float SoftCutVoice::getPos() {
    return static_cast<float>(sch.getActivePhase() / sampleRate);
}
