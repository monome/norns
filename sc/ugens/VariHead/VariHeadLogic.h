//
// Created by ezra on 2/23/18.
//

#ifndef VARIHEAD_VARIHEADLOGIC_H
#define VARIHEAD_VARIHEADLOGIC_H

#include <samplerate.h>

class VariHeadLogic {

public:
    void init();
    void deinit();
    void setSampleRate(float sr);

    void setBuffer(float *buf, unsigned int numFrames);

    void setLoopStartSeconds(float t);
    void setLoopEndSeconds(float t);


    void setRate(float r);
    void setPre(float level);
    void setLoopFlag(bool loop);
    float nextSample(const float *in);

    float processBlock(const float *in, int numFrames);

private:
    void resetPhase();
private:
    float sampleRate;
    float *buf;
    int bufFrames;
    int writeIdx;
    float rate; // per-sample phase increment
    float phase; // current logical write phase (for output)
    int start; // loop start (sample index)
    int end; // loop end (sample index)
    int dur; // loop duration (num samples)
    bool runState;
    //enum { WRITE_RESAMP_QUALITY = SRC_SINC_FASTEST };
    /// i think this lets
    enum { WRITE_RESAMP_QUALITY = SRC_LINEAR };
    // for block processing, we need to store up to (blocksize * rate) output samples!
    enum { WRITE_BUF_LEN = 1024 * 32, MAX_RATE = 32 };
    // TODO: store unused input samples for later (?)
    enum { STORE_BUF_LEN = 1024 };
    float writeBuf[WRITE_BUF_LEN]; // temp buffer for resampling write
    float storeBuf[STORE_BUF_LEN]; // buffer for storing unused input samples
    int numStoredFrames;
    SRC_STATE* srcState; // write resampler
    SRC_DATA srcData;

};


#endif //VARIHEAD_VARIHEADLOGIC_H
