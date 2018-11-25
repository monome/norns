/* ------------------------------------------------------------
name: "BrickwallLowpass"
Code generated with Faust 2.5.30 (https://faust.grame.fr)
Compilation options: cpp, -scal -ftz 0

output edit for efficiency - don't regenerate!

------------------------------------------------------------ */

#ifndef  _FAUST_LOWPASSBRICKWALL_H_
#define  _FAUST_LOWPASSBRICKWALL_H_

#include <algorithm>

using std::max;
using std::min;

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <cmath>
#include <math.h>


#ifndef FAUSTCLASS 
#define FAUSTCLASS mydsp
#endif
#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class FaustLowpassBrickwall {

	static float mydsp_faustpower2_f(float value) {
		return (value * value);
	}
 private:
	
	int fSamplingFreq;
	float fConst0;
	float fConst1;
	float fConst2;
	float fConst3;
	float fConst4;
	float fConst5;
	float fConst6;
	float fRec2[3];
	float fConst7;
	float fRec1[3];
	float fConst8;
	float fRec0[3];
	
 public:
	 void instanceConstants(int samplingFreq) {
		fSamplingFreq = samplingFreq;
		fConst0 = tanf((50265.4844f / min(192000.0f, max(1.0f, float(fSamplingFreq)))));
		fConst1 = (1.0f / fConst0);
		fConst2 = (1.0f / (((fConst1 + 0.517638087f) / fConst0) + 1.0f));
		fConst3 = (1.0f / (((fConst1 + 1.41421354f) / fConst0) + 1.0f));
		fConst4 = (1.0f / (((fConst1 + 1.93185163f) / fConst0) + 1.0f));
		fConst5 = (2.0f * (1.0f - (1.0f / mydsp_faustpower2_f(fConst0))));
		fConst6 = (((fConst1 + -1.93185163f) / fConst0) + 1.0f);
		fConst7 = (((fConst1 + -1.41421354f) / fConst0) + 1.0f);
		fConst8 = (((fConst1 + -0.517638087f) / fConst0) + 1.0f);
		
	}

	 void instanceClear() {
		for (int l0 = 0; (l0 < 3); l0 = (l0 + 1)) {
			fRec2[l0] = 0.0f;
			
		}
		for (int l1 = 0; (l1 < 3); l1 = (l1 + 1)) {
			fRec1[l1] = 0.0f;
			
		}
		for (int l2 = 0; (l2 < 3); l2 = (l2 + 1)) {
			fRec0[l2] = 0.0f;
			
		}
		
	}
	
	 void init(int samplingFreq) {
		instanceInit(samplingFreq);
	}
	 void instanceInit(int samplingFreq) {
		instanceConstants(samplingFreq);
		instanceClear();
	}

	 void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* output0 = outputs[0];
		for (int i = 0; (i < count); i = (i + 1)) {
            fRec2[0] = (input0[i] - (fConst4 * ((fConst5 * fRec2[1]) + (fConst6 * fRec2[2]))));
            fRec1[0] = ((fConst4 * (fRec2[2] + (fRec2[0] + (2.0f * fRec2[1])))) -
                        (fConst3 * ((fConst5 * fRec1[1]) + (fConst7 * fRec1[2]))));
            fRec0[0] = ((fConst3 * (fRec1[2] + (fRec1[0] + (2.0f * fRec1[1])))) -
                        (fConst2 * ((fConst5 * fRec0[1]) + (fConst8 * fRec0[2]))));
            output0[i] = fConst2 * (fRec0[2] + (fRec0[0] + (2.0f * fRec0[1])));
            fRec2[2] = fRec2[1];
            fRec2[1] = fRec2[0];
            fRec1[2] = fRec1[1];
            fRec1[1] = fRec1[0];
            fRec0[2] = fRec0[1];
            fRec0[1] = fRec0[0];
        }
	}
};

class LowpassBrickwall { 
    FaustLowpassBrickwall dsp_;
public:        
    void init(int sr) {
        dsp_.instanceConstants(sr);
        dsp_.instanceClear();
    }

    void processSample(float* x) {
        dsp_.compute(1, &x, &x);
    }
};

#endif
