//
// Created by ezra on 12/6/17.
//

#ifndef CUTFADEVOICE_CUTFADEVOICELOGIC_H
#define CUTFADEVOICE_CUTFADEVOICELOGIC_H

#include <cstdint>
#include "SubHead.h"

namespace  softcuthead {

    class SoftCutHeadLogic {

    public:
        SoftCutHeadLogic();

        void init();

        void setSampleRate(float sr);

        void setBuffer(float *buf, uint32_t size);

        void setRate(float x);              // set the playback rate (as a ratio)
        void setLoopStartSeconds(float x);  // set the Voice endpoint in seconds
        void setLoopEndSeconds(float x);    // set the Voice start point in seconds
        void nextSample(float in, float *outPhase, float *outTrig, float *outAudio); // per-sample update function
        void setFadeTime(float secs);

        void setLoopFlag(bool val);

        void setRec(float x);

        void setPre(float x);

        void setFadePre(float x);

        void setFadeRec(float x);

        void setRecRun(bool val);

        void setRecOffset(float x);

        /// FIXME: this method accepts samples and doesn't wrap.
        /// should add something like cutToPos(seconds)

        void cutToPhase(float newPhase); // fade in to new position (given in samples)
    private:
        void updatePhase(int id);

        void updateFade(int id);

        void doneFadeIn(int id);

        void doneFadeOut(int id);

        float peek(double phase); // lookup an audio sample from the buffer
        float peek4(double phase); // interpolated
        void poke(float x, double phase, float fade); // write an audio sample to the buffer
        void poke2(float x, double phase, float fade); // interpolated
        float mixFade(float x, float y, float a, float b); // mix two inputs with phases
    public:
        typedef enum {
            FADE_LIN, FADE_EQ, FADE_EXP
        } fade_t;
    private:

        float sr;           // sample rate
        float *buf;   // audio buffer (allocated elsewhere)
        int bufFrames;      // samples in buffer
        float start;        // start/end points
        float end;
        float fadeInc;      // linear fade increment per sample
        double phaseInc;     // phase_rd increment per sample

        SubHead head[2];

//        double phase[2];     // current buffer read phase
//        float fade[2];      // current playback fade phase
//        int state[2];       // active/inactive/fading state of each head
        //float trig[2]; // trigger output value

        int active;     // current active play head (0 or 1)
        bool loopFlag;  // set to loop, unset for 1-shot

        fade_t fadeMode; // type of fade to use
        float pre; // pre-record level
        float rec; // record level
        float fadePre; // pre-level modulated by xfade
        float fadeRec; // record level modulated by xfade
        bool recRun;
        double recPhaseOffset;
    };

}
#endif //CUTFADEVOICE_CUTFADEVOICELOGIC_H
