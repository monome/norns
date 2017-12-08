//
// Created by ezra on 12/6/17.
//

#ifndef CUTFADELOOP_CUTFADELOOPLOGIC_H
#define CUTFADELOOP_CUTFADELOOPLOGIC_H

#include <cstdint>

class CutFadeLoopLogic {

public:
    CutFadeLoopLogic();
    void setSampleRate(float sr);
    void setBuffer(const float* buf, uint32_t size);
    void setRate(float x);              // set the playback rate (as a ratio)
    void setLoopStartSeconds(float x);  // set the loop endpoint in seconds
    void setLoopEndSeconds(float x);    // set the loop start point in seconds
    //void setPosSeconds(float x);        // immediately cut to a new position, initiating crossfade
    void nextSample(float* outAudio, float* outPhase); // per-sample update function
    void setFadeTime(float secs);
    void setLoopFlag(bool val);
    void resetPos();
private:
    void updatePhase(int id);
    void updateFade(int id);
    void cutToPos(float newPhase); // immediately cut to a new position in samples
    void doneFadeIn(int id);
    void doneFadeOut(int id);
    float peek(float phase); // lookup an audio sample from the buffer (interpolated)
private:
    enum { ACTIVE, INACTIVE, FADEIN, FADEOUT };
    float sr;           // sample rate
    const float* buf;         // audio buffer (allocated elsewhere)
    int bufFrames;        // samples in buffer
    float start, end;   // loop points
    float fadeInc;      // linear fade increment per sample
    float phaseInc;     // phase increment per sample
    float phase[2];     // current buffer phase
    float fade[2];      // current fade phase
    int state[2];       // active/inactive/fading state of each head
    int active;     // current active play head (0 or 1)
    bool loopFlag;  // set to loop, unset for 1-shot

};


#endif //CUTFADELOOP_CUTFADELOOPLOGIC_H