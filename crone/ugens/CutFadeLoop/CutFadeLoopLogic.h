//
// Created by ezra on 12/6/17.
//

#ifndef CUTFADELOOP_CUTFADELOOPLOGIC_H
#define CUTFADELOOP_CUTFADELOOPLOGIC_H

#include <functional>


class CutFadeLoopLogic {

public:
    CutFadeLoopLogic(float sr, float* buf, int bufSize);
    void setRate(float x);              // set the playback rate (as a ratio)
    void setLoopStartSeconds(float x);  // set the loop endpoint in seconds
    void setLoopEndSeconds(float x);    // set the loop start point in seconds
    void setPosSeconds(float x);        // immediately cut to a new position, initiating crossfade
    void nextSample(float* outAudio, float* outPhase); // per-sample update function

private:
    void updatePhase(int id);
    void cutToPos(float newPhase); // immediately cut to a new position in samples

private:
    float sr;           // sample rate
    float* buf;         // audio buffer (allocated elsewhere)
    int bufSize;        // samples in buffer
    float start, end;   // loop points
    float phi;          // phase increment per sample
    float phase[2];     // current phase of read heads
    float amp[2];       // current volume of read heads
    int state[2];
    int active;     // current active play head (0 or 1)

};


#endif //CUTFADELOOP_CUTFADELOOPLOGIC_H