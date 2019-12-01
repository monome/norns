#pragma once

#include "Utilities.h"
#include "Taper.h"

namespace crone { 

    class PeakMeter {
	float val;
	Slew slew;
	
    public:
	PeakMeter() :
	    val(0),
	    // hackery: hard-coded block size, samplerate, and rise/fall
	    slew(48000/128, 0, 0.3)
	{ }
	
	void update(float *src, size_t numFrames) {
	    float max = getMax(src, numFrames);	    
	    val = slew.process(max);
	}

	float get() const { return val; }
	float getPos() const { return Taper::Vu::getPos(val); }
	
    private:    
	// get max value in audio block
	static float getMax(float *src, size_t numFrames) {
	    float fmax = 0.f;
	    float f;
	    for (size_t i = 0; i < numFrames; ++i) {
		f = fabsf(src[i]);
		if (f > fmax) { fmax = f; }
	    }
	    return fmax;
	}


    };
}
