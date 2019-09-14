//
// Created by emb on 11/10/18.
//

#ifndef SOFTCUT_SOFTCUT_H
#define SOFTCUT_SOFTCUT_H


#include <thread>

#include "SoftCutVoice.h"
#include "FadeCurves.h"

namespace softcut {

    template<int numVoices>
    class SoftCut {

    private:
        SoftCutVoice scv[numVoices];

        void init() {
            FadeCurves::setPreShape(FadeCurves::Shape::Linear);
            FadeCurves::setRecShape(FadeCurves::Shape::Raised);
            FadeCurves::setMinPreWindowFrames(0);
            FadeCurves::setMinRecDelayFrames(0);
            FadeCurves::setPreWindowRatio(1.f / 8);
            FadeCurves::setRecDelayRatio(1.f / (8 * 16));
        }

    public:
        SoftCut() {
            this->init();
            this->reset();
        }

        void reset() {
            for (int v = 0; v < numVoices; ++v) {
		        scv[v].reset();
                /* scv[v].phase_quant(i, 1); */
                /* scv[v].phase_offset(i, 0); */
            };
        }

        // assumption: channel count is equal to voice count!
        void processBlock(int v, const float *in, float *out, int numFrames) {
            scv[v].processBlockMono(in, out, numFrames);
        }

        void setSampleRate(unsigned int hz) {
            for (auto &v : scv) {
                v.setSampleRate(hz);
            }
        }

        void setRate(int voice, float rate) {
            scv[voice].setRate(rate);
        }

        void setLoopStart(int voice, float sec) {
            scv[voice].setLoopStart(sec);
        }

        void setLoopEnd(int voice, float sec) {
            scv[voice].setLoopEnd(sec);
        }

        void setLoopFlag(int voice, bool val) {
            scv[voice].setLoopFlag(val);
        }

        void setFadeTime(int voice, float sec) {
            scv[voice].setFadeTime(sec);
        }

        void setRecLevel(int voice, float amp) {
            scv[voice].setRecLevel(amp);
        }

        void setPreLevel(int voice, float amp) {
            scv[voice].setPreLevel(amp);
        }

        void setRecFlag(int voice, bool val) {
            scv[voice].setRecFlag(val);
        }

        void setPlayFlag(int voice, bool val) {
            scv[voice].setPlayFlag(val);
        }

        void cutToPos(int voice, float sec) {
            scv[voice].cutToPos(sec);
        }

        void setPreFilterFc(int voice, float x) {
            scv[voice].setPreFilterFc(x);
        }

        void setPreFilterRq(int voice, float x) {
            scv[voice].setPreFilterRq(x);
        }

        void setPreFilterLp(int voice, float x) {
            scv[voice].setPreFilterLp(x);
        }

        void setPreFilterHp(int voice, float x) {
            scv[voice].setPreFilterHp(x);
        }

        void setPreFilterBp(int voice, float x) {
            scv[voice].setPreFilterBp(x);
        }

        void setPreFilterBr(int voice, float x) {
            scv[voice].setPreFilterBr(x);
        }

        void setPreFilterDry(int voice, float x) {
            scv[voice].setPreFilterDry(x);
        }

        void setPreFilterFcMod(int voice, float x) {
            scv[voice].setPreFilterFcMod(x);
        }

        void setPostFilterFc(int voice, float x) {
            scv[voice].setPostFilterFc(x);
        }

        void setPostFilterRq(int voice, float x) {
            scv[voice].setPostFilterRq(x);
        }

        void setPostFilterLp(int voice, float x) {
            scv[voice].setPostFilterLp(x);
        }

        void setPostFilterHp(int voice, float x) {
            scv[voice].setPostFilterHp(x);
        }

        void setPostFilterBp(int voice, float x) {
            scv[voice].setPostFilterBp(x);
        }

        void setPostFilterBr(int voice, float x) {
            scv[voice].setPostFilterBr(x);
        }

        void setPostFilterDry(int voice, float x) {
            scv[voice].setPostFilterDry(x);
        }

#if 0 // not allowing realtime manipulation of fade logic params
        void setPreFadeWindow(float x) {
            auto t = std::thread([x] {
                FadeCurves::setPreWindowRatio(x);
            });
            t.detach();
        }

        void setRecFadeDelay(float x) {
            auto t = std::thread([x] {
                FadeCurves::setRecDelayRatio(x);
            });
            t.detach();
        }

        void setPreFadeShape(float x) {
            auto t = std::thread([x] {
                FadeCurves::setPreShape(static_cast<FadeCurves::Shape>(x));
            });
            t.detach();
        }

        void setRecFadeShape(float x) {
            auto t = std::thread([x] {
                FadeCurves::setRecShape(static_cast<FadeCurves::Shape>(x));
            });
            t.detach();
        }
#endif

        void setRecOffset(int i, float d) {
            scv[i].setRecOffset(d);
        }

        void setLevelSlewTime(int i, float d) {
            scv[i].setLevelSlewTime(d);
        }

        void setRecPreSlewTime(int i, float d) {
            scv[i].setRecPreSlewTime(d);
        }

        void setRateSlewTime(int i, float d) {
            scv[i].setRateSlewTime(d);
        }

        phase_t getQuantPhase(int i) {
            return scv[i].getQuantPhase();
        }

        void setPhaseQuant(int i, phase_t q) {
            scv[i].setPhaseQuant(q);
        }

        void setPhaseOffset(int i, float sec) {
            scv[i].setPhaseOffset(sec);
        }

        bool getRecFlag(int i) {
            return scv[i].getRecFlag();
        }

        bool getPlayFlag(int i) {
            return scv[i].getPlayFlag();
        }

        void syncVoice(int follow, int lead, float offset) {
            scv[follow].cutToPos(scv[lead].getPos() + offset);
        }

        void setVoiceBuffer(int id, float *buf, size_t bufFrames) {
            scv[id].setBuffer(buf, bufFrames);
        }

    };
}


#endif //SOFTCUT_SOFTCUT_H2
