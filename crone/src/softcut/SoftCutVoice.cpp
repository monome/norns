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
    svfPreFcBase = 16000;
    reset();
}

void SoftCutVoice::reset() {
    svfPre.setLpMix(1.0);
    svfPre.setHpMix(0.0);
    svfPre.setBpMix(0.0);
    svfPre.setBrMix(0.0);
    svfPre.setRq(4.0);
    svfPre.setFc(svfPreFcBase);
    svfPreFcMod = 1.0;
    svfPreDryLevel = 0.0;

    svfPost.setLpMix(0.0);
    svfPost.setHpMix(0.0);
    svfPost.setBpMix(0.0);
    svfPost.setBrMix(0.0);
    svfPost.setRq(4.0);
    svfPost.setFc(12000);
    svfPostDryLevel = 1.0;

    setRecPreSlewTime(0.001);
    setRateSlewTime(0.001);

    recFlag = false;
    playFlag = false;

    sch.init();

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

    float x, y;
    for(int i=0; i<numFrames; ++i) {
        x = svfPre.getNextSample(in[i]) + in[i]*svfPreDryLevel;
        sch.setRate(rateRamp.update());
        sch.setPre(preRamp.update());
        sch.setRec(recRamp.update());
        sampleFunc(x, &y);
	    out[i] = svfPost.getNextSample(y) + y*svfPostDryLevel;
        updateQuantPhase();
    }
}

void SoftCutVoice::setSampleRate(float hz) {
    sampleRate = hz;
    rateRamp.setSampleRate(hz);
    preRamp.setSampleRate(hz);
    recRamp.setSampleRate(hz);
    sch.setSampleRate(hz);
    svfPre.setSampleRate(hz);
    svfPost.setSampleRate(hz);
}

void SoftCutVoice::setRate(float rate) {
    rateRamp.setTarget(rate);
    updatePreSvfFc();
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

// input filter
void SoftCutVoice::setPreFilterFc(float x) {
    svfPreFcBase = x;
    updatePreSvfFc();
}

void SoftCutVoice::setPreFilterRq(float x) {
    svfPre.setRq(x);
}

void SoftCutVoice::setPreFilterLp(float x) {
    svfPre.setLpMix(x);
}

void SoftCutVoice::setPreFilterHp(float x) {
    svfPre.setHpMix(x);
}

void SoftCutVoice::setPreFilterBp(float x) {
    svfPre.setBpMix(x);
}

void SoftCutVoice::setPreFilterBr(float x) {
    svfPre.setBrMix(x);
}

void SoftCutVoice::setPreFilterDry(float x) {
    svfPreDryLevel = x;
}

void SoftCutVoice::setPreFilterFcMod(float x) {
    svfPreFcMod = x;
}

void SoftCutVoice::updatePreSvfFc() {
    float fcMod = std::min(svfPreFcBase, svfPreFcBase * std::fabs(static_cast<float>(sch.getRate())));
    fcMod = svfPreFcBase + svfPreFcMod * (fcMod - svfPreFcBase);
    svfPre.setFc(fcMod);
}

// output filter
void SoftCutVoice::setPostFilterFc(float x) {
    svfPost.setFc(x);
}

void SoftCutVoice::setPostFilterRq(float x) {
    svfPost.setRq(x);
}

void SoftCutVoice::setPostFilterLp(float x) {
    svfPost.setLpMix(x);
}

void SoftCutVoice::setPostFilterHp(float x) {
    svfPost.setHpMix(x);
}

void SoftCutVoice::setPostFilterBp(float x) {
    svfPost.setBpMix(x);
}

void SoftCutVoice::setPostFilterBr(float x) {
    svfPost.setBrMix(x);
}

void SoftCutVoice::setPostFilterDry(float x) {
    // FIXME
    svfPostDryLevel = x;
}

void SoftCutVoice::setBuffer(float *b, unsigned int nf) {
    buf = b;
    bufFrames = nf;
    sch.setBuffer(buf, bufFrames);
}

void SoftCutVoice::setRecOffset(float d) {
    sch.setRecOffsetSamples(static_cast<int>(d * sampleRate));
}

void SoftCutVoice::setRecPreSlewTime(float d) {
    recRamp.setTime(d);
    preRamp.setTime(d);
}

void SoftCutVoice::setRateSlewTime(float d) {
    rateRamp.setTime(d);
}

void SoftCutVoice::setPhaseQuant(float x) {
    phaseQuant = x;
}

void SoftCutVoice::setPhaseOffset(float x) {
    phaseOffset = x * sampleRate;
}


phase_t SoftCutVoice::getQuantPhase() {
    return quantPhase;
}

void SoftCutVoice::updateQuantPhase() {
    if (phaseQuant == 0) {
        quantPhase = sch.getActivePhase() / sampleRate;
    } else {
        quantPhase = std::floor( (sch.getActivePhase() + phaseOffset) /
            (sampleRate *phaseQuant)) * phaseQuant;
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
