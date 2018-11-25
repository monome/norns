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

    public:
        enum { bufFrames = 16777216 };

    private:
        SoftCutVoice scv[numVoices];
        float buf[bufFrames];

        void init() {
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

    public:
        SoftCut() : buf{} {
            this->init();
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
            scv[voice].setRate( rate);
        }

        void setLoopStart(int voice, float sec) {
            scv[voice].setLoopStart( sec);
        }

        void setLoopEnd(int voice, float sec) {
            scv[voice].setLoopEnd( sec);
        }

        void setLoopFlag(int voice, bool val) {
            scv[voice].setLoopFlag( val);
        }

        void setFadeTime(int voice, float sec) {
            scv[voice].setFadeTime( sec);
        }

        void setRecLevel(int voice, float amp) {
            scv[voice].setRecLevel( amp);
        }

        void setPreLevel(int voice, float amp) {
            scv[voice].setPreLevel( amp);
        }

        void setRecFlag(int voice, bool val) {
            scv[voice].setRecFlag( val);
        }

        void cutToPos(int voice, float sec) {
            scv[voice].cutToPos(sec);
        }


        void setFilterFc(int voice, float x) {
            scv[voice].setFilterFc(x);
        }

        void setFilterRq(int voice, float x) {
            scv[voice].setFilterRq(x);
        }

        void setFilterLp(int voice, float x) {
            scv[voice].setFilterLp(x);
        }

        void setFilterHp(int voice, float x) {
            scv[voice].setFilterHp(x);
        }

        void setFilterBp(int voice, float x) {
            scv[voice].setFilterBp(x);
        }

        void setFilterBr(int voice, float x) {
            scv[voice].setFilterBr(x);
        }

        void setFilterDry(int voice, float x) {
            scv[voice].setFilterDry(x);
        }

        void setFilterFcMod(int voice, float x) {
            scv[voice].setFilterFcMod( x);
        }

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

        void setRecOffset(int i, float d) {
            scv[i].setRecOffset(d);
        }

        void setLevelSlewTime(int i, float d) {
            scv[i].setLevelSlewTime(d);
        }

        void setRateSlewTime(int i, float d) {
            scv[i].setRateSlewTime(d);
        }

        void printTestBuffers() {
            scv[0].printTestBuffers();
        }

    };
}


#endif //SOFTCUT_SOFTCUT_H
