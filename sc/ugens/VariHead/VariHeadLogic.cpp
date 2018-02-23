//
// Created by ezra on 2/23/18.
//

#include "VariHeadLogic.h"

void VariHeadLogic::init() {
    int err;
    wrConv = src_new(WRITE_RESAMP_QUALITY, 1, &err);
}

void VariHeadLogic::setSampleRate(float sr) {
    sampleRate = sr;
}

void VariHeadLogic::setBuffer(float *buf, unsigned int numFrames) {

}

void VariHeadLogic::setLoopStartSeconds(float t) {

}

void VariHeadLogic::setLoopEndSeconds(float t) {

}

void VariHeadLogic::setRate(float r) {

}

void VariHeadLogic::setPre(const float level) {

}

void VariHeadLogic::setLoopFlag(bool loop) {

}

float VariHeadLogic::nextSample(const float in) {
    return 0;
}
