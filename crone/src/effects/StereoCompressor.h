/* ------------------------------------------------------------
name: "StereoCompressor"
Code generated with Faust 2.7.4 (https://faust.grame.fr)
Compilation options: cpp, -scal -ftz 0
------------------------------------------------------------ */

#ifndef  _STEREO_COMPRESSOR_H_
#define  _STEREO_COMPRESSOR_H_

#include <string.h>

#include "faust/gui/APIUI.h"
#include "faust/gui/meta.h"
#include "faust/dsp/dsp.h"

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <math.h>


#ifndef FAUSTCLASS 
#define FAUSTCLASS mydsp
#endif
#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class StereoCompressor_dsp : public dsp {
	
 private:
	
	FAUSTFLOAT fVslider0;
	FAUSTFLOAT fVslider1;
	int fSamplingFreq;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fVslider2;
	FAUSTFLOAT fVslider3;
	float fConst2;
	FAUSTFLOAT fVslider4;
	float fRec2[2];
	float fRec1[2];
	FAUSTFLOAT fVslider5;
	float fRec0[2];
	
 public:
	
	void metadata(Meta* m) { 
		m->declare("analyzers.lib/name", "Faust Analyzer Library");
		m->declare("analyzers.lib/version", "0.0");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "0.0");
		m->declare("compressors.lib/name", "Faust Compressor Effect Library");
		m->declare("compressors.lib/version", "0.0");
		m->declare("effect.lib/author", "Julius O. Smith (jos at ccrma.stanford.edu)");
		m->declare("effect.lib/copyright", "Julius O. Smith III");
		m->declare("effect.lib/deprecated", "This library is deprecated and is not maintained anymore. It will be removed in August 2017.");
		m->declare("effect.lib/exciter_author", "Priyanka Shekar (pshekar@ccrma.stanford.edu)");
		m->declare("effect.lib/exciter_copyright", "Copyright (c) 2013 Priyanka Shekar");
		m->declare("effect.lib/exciter_license", "MIT License (MIT)");
		m->declare("effect.lib/exciter_name", "Harmonic Exciter");
		m->declare("effect.lib/exciter_version", "1.0");
		m->declare("effect.lib/license", "STK-4.3");
		m->declare("effect.lib/name", "Faust Audio Effect Library");
		m->declare("effect.lib/version", "1.33");
		m->declare("filename", "StereoCompressor");
		m->declare("filter.lib/author", "Julius O. Smith (jos at ccrma.stanford.edu)");
		m->declare("filter.lib/copyright", "Julius O. Smith III");
		m->declare("filter.lib/deprecated", "This library is deprecated and is not maintained anymore. It will be removed in August 2017.");
		m->declare("filter.lib/license", "STK-4.3");
		m->declare("filter.lib/name", "Faust Filter Library");
		m->declare("filter.lib/reference", "https://ccrma.stanford.edu/~jos/filters/");
		m->declare("filter.lib/version", "1.29");
		m->declare("math.lib/author", "GRAME");
		m->declare("math.lib/copyright", "GRAME");
		m->declare("math.lib/deprecated", "This library is deprecated and is not maintained anymore. It will be removed in August 2017.");
		m->declare("math.lib/license", "LGPL with exception");
		m->declare("math.lib/name", "Math Library");
		m->declare("math.lib/version", "1.0");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.1");
		m->declare("music.lib/author", "GRAME");
		m->declare("music.lib/copyright", "GRAME");
		m->declare("music.lib/deprecated", "This library is deprecated and is not maintained anymore. It will be removed in August 2017.");
		m->declare("music.lib/license", "LGPL with exception");
		m->declare("music.lib/name", "Music Library");
		m->declare("music.lib/version", "1.0");
		m->declare("name", "StereoCompressor");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/version", "0.0");
	}

	virtual int getNumInputs() {
		return 2;
		
	}
	virtual int getNumOutputs() {
		return 2;
		
	}
	virtual int getInputRate(int channel) {
		int rate;
		switch (channel) {
			case 0: {
				rate = 1;
				break;
			}
			case 1: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}
			
		}
		return rate;
		
	}
	virtual int getOutputRate(int channel) {
		int rate;
		switch (channel) {
			case 0: {
				rate = 1;
				break;
			}
			case 1: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}
			
		}
		return rate;
		
	}
	
	static void classInit(int samplingFreq) {
        (void)samplingFreq;
	}
	
	virtual void instanceConstants(int samplingFreq) {
		fSamplingFreq = samplingFreq;
		fConst0 = std::min(192000.0f, std::max(1.0f, float(fSamplingFreq)));
		fConst1 = (2.0f / fConst0);
		fConst2 = (1.0f / fConst0);
		
	}
	
	virtual void instanceResetUserInterface() {
		fVslider0 = FAUSTFLOAT(0.0f);
		fVslider1 = FAUSTFLOAT(0.0f);
		fVslider2 = FAUSTFLOAT(0.01f);
		fVslider3 = FAUSTFLOAT(4.0f);
		fVslider4 = FAUSTFLOAT(0.10000000000000001f);
		fVslider5 = FAUSTFLOAT(-6.0f);
		
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fRec2[l0] = 0.0f;
			
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fRec1[l1] = 0.0f;
			
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			fRec0[l2] = 0.0f;
			
		}
		
	}
	
	virtual void init(int samplingFreq) {
		classInit(samplingFreq);
		instanceInit(samplingFreq);
	}
	virtual void instanceInit(int samplingFreq) {
		instanceConstants(samplingFreq);
		instanceResetUserInterface();
		instanceClear();
	}
	
	virtual StereoCompressor_dsp* clone() {
		return new StereoCompressor_dsp();
	}
	virtual int getSampleRate() {
		return fSamplingFreq;
		
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("StereoCompressor");
		ui_interface->declare(&fVslider3, "1", "");
		ui_interface->addVerticalSlider("ratio", &fVslider3, 4.0f, 1.0f, 100.0f, 0.100000001f);
		ui_interface->declare(&fVslider5, "2", "");
		ui_interface->declare(&fVslider5, "unit", "dB");
		ui_interface->addVerticalSlider("threshold", &fVslider5, -6.0f, -40.0f, 0.0f, 0.5f);
		ui_interface->declare(&fVslider2, "3", "");
		ui_interface->declare(&fVslider2, "scale", "log");
		ui_interface->declare(&fVslider2, "unit", "s");
		ui_interface->addVerticalSlider("attack", &fVslider2, 0.00999999978f, 0.00100000005f, 4.0f, 0.00100000005f);
		ui_interface->declare(&fVslider4, "4", "");
		ui_interface->declare(&fVslider4, "scale", "log");
		ui_interface->declare(&fVslider4, "unit", "s");
		ui_interface->addVerticalSlider("release", &fVslider4, 0.100000001f, 0.00100000005f, 4.0f, 0.00100000005f);
		ui_interface->declare(&fVslider0, "5", "");
		ui_interface->declare(&fVslider0, "unit", "dB");
		ui_interface->addVerticalSlider("gain_pre", &fVslider0, 0.0f, -24.0f, 24.0f, 0.5f);
		ui_interface->declare(&fVslider1, "6", "");
		ui_interface->declare(&fVslider1, "unit", "dB");
		ui_interface->addVerticalSlider("gain_post", &fVslider1, 0.0f, -24.0f, 24.0f, 0.5f);
		ui_interface->closeBox();
		
	}
	
	virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = std::pow(10.0f, (0.0500000007f * float(fVslider0)));
		float fSlow1 = (fSlow0 * std::pow(10.0f, (0.0500000007f * float(fVslider1))));
		float fSlow2 = float(fVslider2);
		float fSlow3 = std::exp((0.0f - (fConst1 / fSlow2)));
		float fSlow4 = (((1.0f / float(fVslider3)) + -1.0f) * (1.0f - fSlow3));
		float fSlow5 = std::exp((0.0f - (fConst2 / fSlow2)));
		float fSlow6 = std::exp((0.0f - (fConst2 / float(fVslider4))));
		float fSlow7 = float(fVslider5);
		for (int i = 0; (i < count); i = (i + 1)) {
			float fTemp0 = float(input1[i]);
			float fTemp1 = float(input0[i]);
			float fTemp2 = std::fabs((std::fabs((fSlow0 * fTemp0)) + std::fabs((fSlow0 * fTemp1))));
			float fTemp3 = ((fRec1[1] > fTemp2)?fSlow6:fSlow5);
			fRec2[0] = ((fTemp3 * fRec2[1]) + ((1.0f - fTemp3) * fTemp2));
			fRec1[0] = fRec2[0];
			fRec0[0] = ((fSlow3 * fRec0[1]) + (fSlow4 * std::max(((20.0f * std::log10(fRec1[0])) - fSlow7), 0.0f)));
			float fTemp4 = std::pow(10.0f, (0.0500000007f * fRec0[0]));
			output0[i] = FAUSTFLOAT((fSlow1 * (fTemp4 * fTemp1)));
			output1[i] = FAUSTFLOAT((fSlow1 * (fTemp4 * fTemp0)));
			fRec2[1] = fRec2[0];
			fRec1[1] = fRec1[0];
			fRec0[1] = fRec0[0];
			
		}
		
	}

	
};

class StereoCompressor
{
  private:

    StereoCompressor_dsp _dsp;
    APIUI ui;

  public:        
    StereoCompressor() {
        _dsp.buildUserInterface(&ui);
    }

    void init(double sampleRate)
    {
        StereoCompressor_dsp::classInit(int(sampleRate));
        _dsp.instanceConstants(int(sampleRate));
        _dsp.instanceClear();
    }

    void processBlock(float** in, float** out, int numSamples)
    {
      _dsp.compute(numSamples, in, out);
    }

    int getNumInputs() { return _dsp.getNumInputs(); }
    int getNumOutputs() { return _dsp.getNumOutputs(); }

    APIUI& getUi() { return ui; }
};

#endif
