//
// Created by ezra on 4/21/18.
//

/*
 * this class implements one half of a crossfaded read/write sch.
 */

#ifndef SOFTCUTHEAD_SUBHEAD_H
#define SOFTCUTHEAD_SUBHEAD_H

#include <boost/math/special_functions/sign.hpp>

#include "Resampler.h"
#include "LowpassBrickwall.h"
#include "SoftClip.h"
#include "Types.h"

namespace softcut {

    typedef enum { Active=0, Inactive=1, FadeIn=2, FadeOut=3 } State;
    typedef enum { None, Stop, LoopPos, LoopNeg } Action ;

    class SubHead {
        friend class SoftCutHead;
    public:
        SubHead();
        void init();
        void setSampleRate(float sr);
    private:
        sample_t peek4();
        unsigned int wrapBufIndex(int x);

    protected:
        sample_t peek();
        //! poke
        //! @param in: input value
        //! @param pre: scaling level for previous buffer content
        //! @param rec: scaling level for new content
        //! @param numFades: number of heads currently in crossfade
        void poke(sample_t in, float pre, float rec, int numFades);
        Action updatePhase(phase_t start, phase_t end, bool loop);
        void updateFade(float inc);

        // getters
        phase_t phase() { return phase_; }
        float fade() { return fade_; }
        float trig() { return trig_; }
        State state() { return state_; }
        
        // setters
        void setState(State state);
        void setTrig(float trig);
        void setPhase(phase_t phase);

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // **NB** buffer size must be a power of two!!!!
        void setBuffer(sample_t *buf, unsigned int frames);
        void setRate(rate_t rate);


    private:
        Resampler resamp_;
        LowpassBrickwall lpf_;
        SoftClip clip_;

        sample_t* buf_; // output buffer
        unsigned int wrIdx_; // write index
        unsigned int bufFrames_;
        unsigned int bufMask_;

        State state_;
        rate_t rate_;
        int inc_dir_;
        phase_t phase_;
        float fade_;
        float trig_; // output trigger value
        bool active_;
        int recOffset_;

        float preFade_;
        float recFade_;

        void setRecOffsetSamples(int d);
    };

}


#endif //SOFTCUTHEAD_SUBHEAD_H
