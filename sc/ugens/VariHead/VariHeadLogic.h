//
// Created by ezra on 2/23/18.
//

#ifndef VARIHEAD_VARIHEADLOGIC_H
#define VARIHEAD_VARIHEADLOGIC_H

#include <samplerate.h>

class VariHeadLogic {

public:
    void init();
    void setSampleRate(float sr);

    void setBuffer(float *buf, unsigned int numFrames);

    void setLoopStartSeconds(float t);
    void setLoopEndSeconds(float t);


    void setRate(float r);

    void setPre(const float level);

    void setLoopFlag(bool loop);

    float nextSample(const float in);

private:
    float sampleRate;
    enum { WRITE_RESAMP_QUALITY = SRC_SINC_FASTEST};
    enum { WRITE_BUF_LEN = 64 };
    float writeBuf[WRITE_BUF_LEN]; // ringbuffer for resampling write
    SRC_STATE* wrConv; // write resampler
};


#endif //VARIHEAD_VARIHEADLOGIC_H
