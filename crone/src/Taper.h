#pragma once

#include "Utilities.h"

class AudioMeter {
 private:
    static const float ampPosTable;
    static const unsigned int ampPosTableSize;
 public:
    // get position on perceptual taper scale (IEC-60268)
    static float getPos(signal) {
	const float amp = std::abs(signal);
	// FIXME: should we return a special value for clipping?
	if (amp > 1.f) { return 1.f; }
	return LUT::lookupLinear(amp, ampPosTable, ampPosTableSize); 
    }
};

    
