//
// Created by ezra on 2/23/18.
//

#include <samplerate.h>
#include <cstdio>
#include <cmath>
#include "VariHeadLogic.h"

void VariHeadLogic::init() {
    int err;
    writeIdx = 0;
    for(int i=0; i<WRITE_BUF_LEN; ++i) { writeBuf[i] = 0.f; }
    srcState = src_new(WRITE_RESAMP_QUALITY, 1, &err);
    srcData.output_frames = WRITE_BUF_LEN;
    srcData.input_frames = 0;
    srcData.src_ratio = 1.0;
    rate = 1.0;
}

void VariHeadLogic::deinit() {
    src_delete(srcState);
}


void VariHeadLogic::setSampleRate(float sr) {
    sampleRate = sr;
}

void VariHeadLogic::setBuffer(float *b, unsigned int n) {
    buf = b;
    bufFrames = n;
}

void VariHeadLogic::setLoopStartSeconds(float t) {
    start = static_cast<int>(t * sampleRate);
    if(start > (bufFrames-1)) { printf("clamped start\n"); start = (bufFrames-1); }
    dur = end - start;
}

void VariHeadLogic::setLoopEndSeconds(float t) {
    end = static_cast<int>(t * sampleRate);
    if(end> (bufFrames-1)) { printf("clamped end\n"); end = (bufFrames-1); }
    dur = end - start;
}

void VariHeadLogic::setRate(float r) {
    rate = r;
    // FIXME: support negative rates
    if(rate < (MAX_RATE * -1)) { rate = (MAX_RATE * -1); }
    if(rate > MAX_RATE) { rate = MAX_RATE; }
    //... SRC's internal ratio interpolation is too slow..
    // try to bypass it (not recommended in API docs)
    src_set_ratio(srcState, abs(rate));
}

void VariHeadLogic::resetPhase() {
    while(phase > end) { phase -= dur; }
}

float VariHeadLogic::processBlock(const float *in, int numFrames) {
    srcData.data_in = in;
    srcData.data_out = writeBuf;
    srcData.input_frames += numFrames;
    // don't do this b/c we are setting the ratio directly in SRC state structure
    // srcData.src_ratio = rate;
    srcData.end_of_input = 0;
    int err = src_process(srcState, &srcData);
    if(err) {
        printf ("src_process error: %s\n", src_strerror (err));
        printf("rate: %f; src_ratio: %f\n", rate, srcData.src_ratio);
        printf("input_frames: %ld; input_frames_used: %ld\n", srcData.input_frames, srcData.input_frames_used);
        printf("output_frames: %ld; output_frames_gen: %ld\n", srcData.output_frames, srcData.output_frames_gen);
    }

    // copy temp buffer to ugen buffer
    if(rate < 0.f) {
        for (int i = 0; i < srcData.output_frames_gen; ++i) {
            buf[writeIdx] = writeBuf[i];
            writeIdx--;
            if (writeIdx < start || writeIdx < 0) {  writeIdx = end; }
        }
    } else {
        for (int i = 0; i < srcData.output_frames_gen; ++i) {
            if (writeIdx < start) {  writeIdx = start; }
            buf[writeIdx] = writeBuf[i];
            writeIdx++;
            if (writeIdx >= end || writeIdx >= bufFrames) {  writeIdx = start;  }
        }
    }
    if(srcData.input_frames_used != numFrames) {
        printf("SRC didn't use %ld input samples this block\n", numFrames - srcData.input_frames_used);
        // FIXME: need to store unused frames for next block?
        // (this never seems to be hit, regardless of interp method)
    }
    srcData.input_frames -= srcData.input_frames_used;
    phase += (numFrames * rate);
    if (phase > end) { resetPhase(); }
    return phase;
}
