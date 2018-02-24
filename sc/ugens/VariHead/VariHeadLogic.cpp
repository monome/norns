//
// Created by ezra on 2/23/18.
//

#include <samplerate.h>
#include <cstdio>
#include "VariHeadLogic.h"

void VariHeadLogic::init() {
    int err;
    writeIdx = 0;
    for(int i=0; i<WRITE_BUF_LEN; ++i) { writeBuf[i] = 0.f; }
    srcState = src_new(WRITE_RESAMP_QUALITY, 1, &err);
    srcData.output_frames = WRITE_BUF_LEN;
    srcData.input_frames = 0;
    numStoredFrames = 0;
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
#if 0 // test: just write to the buffer in the dumbest way
    /// this works as expected
    buf[writeIdx] = *in;
    writeIdx++;
    if(writeIdx > end || writeIdx >= bufFrames) { writeIdx = start; }

#else

    // setup the conversions
    srcData.data_in = in;
    srcData.data_out = writeBuf;
    // FIXME: if we don't use all the input frames, we need to store them for next time.
    srcData.input_frames += 1;
    srcData.src_ratio = rate;
    srcData.end_of_input = 0;

    int err = src_process(srcState, &srcData);
    if(err) {
        printf ("src_process error: %s\n", src_strerror (err));
        printf("rate: %f; src_ratio: %f\n", rate, srcData.src_ratio);
        printf("input_frames: %ld; input_frames_used: %ld\n", srcData.input_frames, srcData.input_frames_used);
        printf("output_frames: %ld; output_frames_gen: %ld\n", srcData.output_frames, srcData.output_frames_gen);
    }

    // copy temp buffer to ugen buffer
    for(int i=0; i<srcData.output_frames_gen; ++i) {
        buf[writeIdx] = writeBuf[i];
        writeIdx++;
        if(writeIdx > end || writeIdx >= bufFrames) { writeIdx = start; }
    }
    srcData.input_frames -= srcData.input_frames_used;
#endif

    // update the phase for output
    phase += rate;
    if(phase > end) { resetPhase(); }
    return phase;
}

void VariHeadLogic::resetPhase() {
    while(phase > end) { phase -= dur; }
}

float VariHeadLogic::processBlock(const float *in, int numFrames) {
    srcData.data_in = in;
    srcData.data_out = writeBuf;
    srcData.input_frames += numFrames;
    srcData.src_ratio = rate;
    srcData.end_of_input = 0;
    int err = src_process(srcState, &srcData);
    if(err) {
        printf ("src_process error: %s\n", src_strerror (err));
        printf("rate: %f; src_ratio: %f\n", rate, srcData.src_ratio);
        printf("input_frames: %ld; input_frames_used: %ld\n", srcData.input_frames, srcData.input_frames_used);
        printf("output_frames: %ld; output_frames_gen: %ld\n", srcData.output_frames, srcData.output_frames_gen);
    }

    // copy temp buffer to ugen buffer
    for(int i=0; i<srcData.output_frames_gen; ++i) {
        buf[writeIdx] = writeBuf[i];
        writeIdx++;
        if(writeIdx > end || writeIdx >= bufFrames) { writeIdx = start; }
    }
    if(srcData.input_frames_used != numFrames) {
     //   printf("uh oh: SRC didn't use %ld input samples this block\n", numFrames - srcData.input_frames_used);
        // FIXME: need to store unused frames for next block?
    }
    srcData.input_frames -= srcData.input_frames_used;
    phase += (numFrames * rate);
    if (phase > end) { resetPhase(); }
    return phase;
}
