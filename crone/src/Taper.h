#pragma once

#include "Utilities.h"

namespace  crone {
    class Taper {
    public:
	class Vu {
	private:
	    static const float ampPosTable[];
	    static const unsigned int ampPosTableSize;
	public:
	    // get position on perceptual taper scale (IEC-60268)
	    // assumption: amp >= 0.f
	    static float getPos(float amp) {
		// FIXME: should we return a special value for clipping?
		if (amp > 1.f) { return 1.f; }
		return LUT<float>::lookupLinear(amp, ampPosTable, ampPosTableSize);
	    }
	};
    };
}
