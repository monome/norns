//
// Created by ezra on 11/15/18.
//

#include <math.h>
#include <algorithm>
#include <iostream>
#include <boost/assert.hpp>
#include <cstring>

#include "Interpolate.h"
#include "FadeCurves.h"

using namespace softcut;

static constexpr float fpi = 3.1415926535898f;

float FadeCurves::recDelayRatio;
float FadeCurves::preWindowRatio;
unsigned int FadeCurves::recDelayMinFrames;
unsigned int FadeCurves::preWindowMinFrames;
float FadeCurves::recFadeBuf[fadeBufSize];
float FadeCurves::preFadeBuf[fadeBufSize];
FadeCurves::Shape FadeCurves::recShape = Linear;
FadeCurves::Shape FadeCurves::preShape = Linear;

void FadeCurves::calcRecFade() {
    float buf[fadeBufSize];
    unsigned int n = fadeBufSize - 1;
    // build rec-fade curve
    // this will be scaled by base rec level
    unsigned int ndr = std::max(recDelayMinFrames,
                                static_cast<unsigned int>(recDelayRatio * fadeBufSize));
    unsigned int nr = n - ndr;

    unsigned int i = 0;
    if(recShape == SINE) {
        const float phi = fpi / nr;
        float x = fpi;
        float y = 0.f;
        while (i < ndr) {
            buf[i++] = y;
        }
        while (i < n) {
            y = cosf(x) * 0.5f + 0.5f;
            buf[i++] = y;
            x += phi;
        }
        buf[n] = 1.f;
    } else if (recShape == Linear) {
        const float phi = 1.f / nr;
        float x = 0.f;
        while (i < ndr) {
            buf[i++] = x;
        }
        while (i < n) {
            buf[i++] = x;
            x += phi;
        }
        buf[n] = 1.f;
    } else if (recShape == Raised) {
        const float phi = fpi/(nr*2);
        float x = fpi;
        float y = 0.f;
        while (i < ndr) {
            buf[i++] = y;
        }
        while (i < n) {
            y = sinf(x);
            buf[i++] = y;
            x += phi;
        }
        buf[n] = 1.f;
    } else {
        BOOST_ASSERT_MSG(false, "undefined fade shape");
    }
    memcpy(recFadeBuf, buf, fadeBufSize*sizeof(float));
}

void FadeCurves::calcPreFade() {
    float buf[fadeBufSize];
    // build pre-fade curve
    // this will be scaled and added to the base pre value (mapping [0, 1] -> [pre, 1])
    unsigned int nwp = std::max(preWindowMinFrames,
                                static_cast<unsigned int>(preWindowRatio * fadeBufSize));

    unsigned int i = 0;
    float x = 0.f;
    if(preShape == SINE) {
        const float phi = fpi / nwp;
        while (i < nwp) {
            buf[i++] = cosf(x) * 0.5f + 0.5f;
            x += phi;
        }
    } else if (preShape == Linear){
        const float phi = 1.f / nwp;
        while (i < nwp) {
            buf[i++] = 1.f - x;
            x += phi;
        }
    } else if (recShape == Raised) {
        BOOST_ASSERT(preShape == Raised);
        const float phi = fpi / (nwp*2);
        while (i < nwp) {
            buf[i++] = cosf(x);
            x += phi;
        }
    } else {
        BOOST_ASSERT_MSG(false, "undefined fade shape");
    }
    while(i<fadeBufSize) { buf[i++] = 0.f; }
    memcpy(preFadeBuf, buf, fadeBufSize*sizeof(float));

}

void FadeCurves::setRecDelayRatio(float x) {
    recDelayRatio = x;
    calcRecFade();
}

void FadeCurves::setPreWindowRatio(float x) {
    preWindowRatio = x;
    calcPreFade();
}

void FadeCurves::setMinRecDelayFrames(unsigned int x) {
    recDelayMinFrames = x;
    calcRecFade();
}

void FadeCurves::setMinPreWindowFrames(unsigned int x) {
    preWindowMinFrames = x;
    calcPreFade();
}

float FadeCurves::getRecFadeValue(float x) {
    return Interpolate::tabLinear<float, fadeBufSize>(recFadeBuf, x);
}


float FadeCurves::getPreFadeValue(float x) {
    return Interpolate::tabLinear<float, fadeBufSize>(preFadeBuf, x);
}

void FadeCurves::setPreShape(FadeCurves::Shape x) {
    preShape = x;
    calcPreFade();
}

void FadeCurves::setRecShape(FadeCurves::Shape x) {
    recShape = x;
    calcRecFade();
}
