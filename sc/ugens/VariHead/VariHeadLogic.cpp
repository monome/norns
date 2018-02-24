//
// Created by ezra on 2/23/18.
//

#include <samplerate.h>
#include "VariHeadLogic.h"

void VariHeadLogic::init() {
    int err;
    writeIdx = 0;
    for(int i=0; i<WRITE_BUF_LEN; ++i) { writeBuf[i] = 0.f; }
    srcState = src_new(WRITE_RESAMP_QUALITY, 1, &err);
    srcData.output_frames = WRITE_BUF_LEN;
}

void VariHeadLogic::deinit() {
    src_delete(srcState);
}


void VariHeadLogic::setSampleRate(float sr) {
    sampleRate = sr;
}

void VariHeadLogic::setBuffer(float *theBuf, unsigned int numFrames) {
    buf = theBuf;
    bufFrames = numFrames;
}

void VariHeadLogic::setLoopStartSeconds(float t) {
    start = static_cast<int>(t * sampleRate);
    dur = end - start;
}

void VariHeadLogic::setLoopEndSeconds(float t) {
    end = static_cast<int>(t * sampleRate);
    dur = end - start;
}

void VariHeadLogic::setRate(float r) {
    rate = r;
    // FIXME: support negative rates
    if(rate < 0.0) { rate = 0.0; }
    if(rate > MAX_RATE) { rate = MAX_RATE; }
}

void VariHeadLogic::setPre(const float level) {

}

void VariHeadLogic::setLoopFlag(bool loop) {
    // TODO
}

float VariHeadLogic::nextSample(const float* in) {
#if 1 // test: just write to the buffer in the dumbest way
    buf[writeIdx] = *in;
    writeIdx++;
    if(writeIdx > end || writeIdx >= bufFrames) { writeIdx = start; }

#else

    // setup the conversions
    srcData.data_in = in;
    srcData.data_out = writeBuf;
    srcData.input_frames = 1;
    srcData.src_ratio = rate;

    // process to temp buffer
    src_process(srcState, &srcData);

    // copy temp buffer to ugen buffer
    for(int i=0; i<srcData.output_frames_gen; ++i) {
        buf[writeIdx] = writeBuf[i];
        writeIdx++;
        if(writeIdx > end || writeIdx >= bufFrames) { writeIdx = start; }
    }
#endif

    // update the phase for output
    phase += rate;
    if(phase > end) { resetPhase(); }
    return phase;
}

void VariHeadLogic::resetPhase() {
    while(phase > end) { phase -= dur; }
//    writeIdx = static_cast<int>(start + phase);
//    src_reset(wrConv);
}
