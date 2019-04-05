//
// Created by ezra on 11/3/18.
//

#ifndef SOFTCUT_SOFTCUTVOICE_H
#define SOFTCUT_SOFTCUTVOICE_H

#include <array>
#include <atomic>

#include "SoftCutHead.h"
#include "Svf.h"
#include "../Utilities.h"

namespace softcut {
    class SoftCutVoice {
    public:
        SoftCutVoice();
        void setBuffer(float* buf, unsigned int numFrames);

        void setSampleRate(float hz);
        void setRate(float rate);
        void setLoopStart(float sec);
        void setLoopEnd(float sec);
        void setLoopFlag(bool val);

        void setFadeTime(float sec);

        void setRecLevel(float amp);
        void setPreLevel(float amp);
        void setRecFlag(bool val);
        void setPlayFlag(bool val);

        void setFilterFc(float);
        void setFilterRq(float);
        void setFilterLp(float);
        void setFilterHp(float);
        void setFilterBp(float);
        void setFilterBr(float);
        void setFilterDry(float);
        void setFilterFcMod(float x);

        void cutToPos(float sec);

        // process a single channel
        void processBlockMono(const float* in, float* out, int numFrames);

        void setRecOffset(float d);
        void setLevelSlewTime(float d);
        void setRateSlewTime(float d);
        void setPhaseQuant(float x);

        phase_t getQuantPhase();

        bool getPlayFlag();
        bool getRecFlag();

        float getPos();

    private:
        void updateFilterFc();
        void updateQuantPhase();

    private:
        float* buf;
        int bufFrames;
        float sampleRate;

        // xfaded read/write head
        SoftCutHead sch;
        // input filter
        Svf svf;
        // rate ramp
        crone::LogRamp rateRamp;
        // pre-level ramp
        crone::LogRamp preRamp;
        // record-level ramp
        crone::LogRamp recRamp;

        // default frequency for SVF
        // reduced automatically when setting rate
        float fcBase;
        // the amount by which SVF frequency is modulated by rate
        float fcMod = 1.0;
        float svfDryLevel = 1.0;
        // phase quantization unit, should be in [0,1]
        phase_t phaseQuant;
        // quantized phase
        std::atomic<phase_t> quantPhase;


    private:

        bool playFlag;
        bool recFlag;

    };
}


#endif //SOFTCUT_SOFTCUTVOICE_H
