//
// Created by ezra on 12/6/17.
//

#ifndef CUTFADEVOICE_CUTFADEVOICELOGIC_H
#define CUTFADEVOICE_CUTFADEVOICELOGIC_H

#include <cstdint>
#include "SubHead.h"
#include "Types.h"
#include "TestBuffers.h"

namespace  softcut{

    class SoftCutHead {
    public:

        SoftCutHead();
        void init();

        // per-sample update functions
        void processSample(sample_t in, sample_t *out);

        void processSampleNoRead(sample_t in, sample_t *out);
        void processSampleNoWrite(sample_t in, sample_t *out);

        void setSampleRate(float sr);
        void setBuffer(sample_t *buf, uint32_t size);
        void setRate(rate_t x);              // set the playback rate (as a ratio)
        void setLoopStartSeconds(float x);  // set the loop end point in seconds
        void setLoopEndSeconds(float x);    // set the loop start point in seconds
        void setFadeTime(float secs);
        void setLoopFlag(bool val);
        void setRec(float x);
        void setPre(float x);
        void cutToPos(float seconds);

        phase_t getActivePhase();
        rate_t getRate();


        void setRecOffsetSamples(int d);
    private:
        // fade in to new position (given in samples)
        // assumption: phase is in range!
        void cutToPhase(phase_t newPhase);
        void enqueueCrossfade(phase_t newPhase);
        void dequeueCrossfade();
        void takeAction(Action act);
        sample_t mixFade(sample_t x, sample_t y, float a, float b); // mix two inputs with phases
        void calcFadeInc();

    private:
        SubHead head[2];

        sample_t *buf;      // audio buffer (allocated elsewhere)
        float sr;           // sample rate
        phase_t start;      // start/end points
        phase_t end;
        phase_t queuedCrossfade;
        bool queuedCrossfadeFlag;
        float fadeTime;     // fade time in seconds
        float fadeInc;      // linear fade increment per sample

        int active;         // current active play head index (0 or 1)
        bool loopFlag;      // set to loop, unset for 1-shot
        float pre;      // pre-record level
        float rec;      // record level
        // record processing flag
        // play processing flag

        rate_t rate;    // current rate
        TestBuffers testBuf;
    };
}
#endif //CUTFADEVOICE_CUTFADEVOICELOGIC_H
