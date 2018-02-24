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
    if(rate < 0.0) { rate = 0.0; }
    if(rate > MAX_RATE) { rate = MAX_RATE; }
    //... SRC's internal ratio interpolation is too slow..
    // try to bypass it (not recommended in API docs)
    src_set_ratio(srcState, rate);
}

//float VariHeadLogic::nextSample(const float* in) {
//#if 0 // test: just write to the buffer in the dumbest way
//    /// this works as expected
//    buf[writeIdx] = *in;
//    writeIdx++;
//    if(writeIdx > end || writeIdx >= bufFrames) { writeIdx = start; }
//
//#else
//    // setup the conversions
//    srcData.data_in = in;
//    srcData.data_out = writeBuf;
//    srcData.input_frames += 1;
//    srcData.src_ratio = rate;
//    srcData.end_of_input = 0;
//
//    int err = src_process(srcState, &srcData);
//    if(err) {
//        printf ("src_process error: %s\n", src_strerror (err));
//        printf("rate: %f; src_ratio: %f\n", rate, srcData.src_ratio);
//        printf("input_frames: %ld; input_frames_used: %ld\n", srcData.input_frames, srcData.input_frames_used);
//        printf("output_frames: %ld; output_frames_gen: %ld\n", srcData.output_frames, srcData.output_frames_gen);
//    }
//
//    // copy temp buffer to ugen buffer
//    for(int i=0; i<srcData.output_frames_gen; ++i) {
//        buf[writeIdx] = writeBuf[i];
//        writeIdx++;
//        if(writeIdx > end || writeIdx >= bufFrames) { writeIdx = start; }
//    }
//    srcData.input_frames -= srcData.input_frames_used;
//    // FIXME: need to store unused frames?
//    // (doesn't seem to be an issue using linear interpolation)
//#endif
//
//    // update the phase for output
//    phase += rate;
//    if(phase > end) { resetPhase(); }
//    return phase;
//}

void VariHeadLogic::resetPhase() {
    while(phase > end) { phase -= dur; }
}

float VariHeadLogic::processBlock(const float *in, int numFrames) {
    srcData.data_in = in;
    srcData.data_out = writeBuf;
    srcData.input_frames += numFrames;
    // test
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
    for(int i=0; i<srcData.output_frames_gen; ++i) {
        if(writeIdx < start) {
            // printf("writeIdx underflow\n");
            writeIdx = start;
        }
        buf[writeIdx] = writeBuf[i];
        writeIdx++;
        if(writeIdx >= end || writeIdx >= bufFrames) {
            // printf("writeIdx overflow\n");
            writeIdx = start;
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
