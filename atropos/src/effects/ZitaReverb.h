/* ------------------------------------------------------------
name: "ZitaReverb"
Code generated with Faust 2.7.4 (https://faust.grame.fr)
Compilation options: cpp, -scal -ftz 0
------------------------------------------------------------ */

#ifndef  _ZITA_REVERB_H_
#define  _ZITA_REVERB_H_

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

static float mydsp_faustpower2_f(float value) {
	return (value * value);
	
}

#ifndef FAUSTCLASS 
#define FAUSTCLASS StereoCompressor_dsp
#endif
#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class ZitaReverb_dsp : public dsp {

 private:

	int fSamplingFreq;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fVslider0;
	float fConst2;
	float fConst3;
	FAUSTFLOAT fVslider1;
	FAUSTFLOAT fVslider2;
	float fConst4;
	FAUSTFLOAT fVslider3;
	float fRec11[2];
	float fRec10[2];
	int IOTA;
	float fVec0[16384];
	float fConst5;
	int iConst6;
	float fVec1[16384];
	float fConst7;
	FAUSTFLOAT fVslider4;
	float fVec2[2048];
	int iConst8;
	float fRec8[2];
	float fConst9;
	float fConst10;
	float fRec15[2];
	float fRec14[2];
	float fVec3[32768];
	float fConst11;
	int iConst12;
	float fVec4[16384];
	float fVec5[2048];
	int iConst13;
	float fRec12[2];
	float fConst14;
	float fConst15;
	float fRec19[2];
	float fRec18[2];
	float fVec6[16384];
	float fConst16;
	int iConst17;
	float fVec7[4096];
	int iConst18;
	float fRec16[2];
	float fConst19;
	float fConst20;
	float fRec23[2];
	float fRec22[2];
	float fVec8[16384];
	float fConst21;
	int iConst22;
	float fVec9[4096];
	int iConst23;
	float fRec20[2];
	float fConst24;
	float fConst25;
	float fRec27[2];
	float fRec26[2];
	float fVec10[32768];
	float fConst26;
	int iConst27;
	float fVec11[4096];
	int iConst28;
	float fRec24[2];
	float fConst29;
	float fConst30;
	float fRec31[2];
	float fRec30[2];
	float fVec12[16384];
	float fConst31;
	int iConst32;
	float fVec13[4096];
	int iConst33;
	float fRec28[2];
	float fConst34;
	float fConst35;
	float fRec35[2];
	float fRec34[2];
	float fVec14[32768];
	float fConst36;
	int iConst37;
	float fVec15[4096];
	int iConst38;
	float fRec32[2];
	float fConst39;
	float fConst40;
	float fRec39[2];
	float fRec38[2];
	float fVec16[16384];
	float fConst41;
	int iConst42;
	float fVec17[2048];
	int iConst43;
	float fRec36[2];
	float fRec0[3];
	float fRec1[3];
	float fRec2[3];
	float fRec3[3];
	float fRec4[3];
	float fRec5[3];
	float fRec6[3];
	float fRec7[3];

 public:

	void metadata(Meta* m) {
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "0.0");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "0.0");
		m->declare("filename", "ZitaReverb");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/version", "0.0");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.1");
		m->declare("name", "ZitaReverb");
		m->declare("reverbs.lib/name", "Faust Reverb Library");
		m->declare("reverbs.lib/version", "0.0");
		m->declare("routes.lib/name", "Faust Signal Routing Library");
		m->declare("routes.lib/version", "0.0");
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
		fConst1 = (6.28318548f / fConst0);
		fConst2 = std::floor(((0.125f * fConst0) + 0.5f));
		fConst3 = ((0.0f - (6.90775537f * fConst2)) / fConst0);
		fConst4 = (3.14159274f / fConst0);
		fConst5 = std::floor(((0.0134579996f * fConst0) + 0.5f));
		iConst6 = int(std::min(8192.0f, std::max(0.0f, (fConst2 - fConst5))));
		fConst7 = (0.00100000005f * fConst0);
		iConst8 = int(std::min(1024.0f, std::max(0.0f, (fConst5 + -1.0f))));
		fConst9 = std::floor(((0.219990999f * fConst0) + 0.5f));
		fConst10 = ((0.0f - (6.90775537f * fConst9)) / fConst0);
		fConst11 = std::floor(((0.0191229992f * fConst0) + 0.5f));
		iConst12 = int(std::min(16384.0f, std::max(0.0f, (fConst9 - fConst11))));
		iConst13 = int(std::min(1024.0f, std::max(0.0f, (fConst11 + -1.0f))));
		fConst14 = std::floor(((0.192303002f * fConst0) + 0.5f));
		fConst15 = ((0.0f - (6.90775537f * fConst14)) / fConst0);
		fConst16 = std::floor(((0.0292910002f * fConst0) + 0.5f));
		iConst17 = int(std::min(8192.0f, std::max(0.0f, (fConst14 - fConst16))));
		iConst18 = int(std::min(2048.0f, std::max(0.0f, (fConst16 + -1.0f))));
		fConst19 = std::floor(((0.174713001f * fConst0) + 0.5f));
		fConst20 = ((0.0f - (6.90775537f * fConst19)) / fConst0);
		fConst21 = std::floor(((0.0229039993f * fConst0) + 0.5f));
		iConst22 = int(std::min(8192.0f, std::max(0.0f, (fConst19 - fConst21))));
		iConst23 = int(std::min(2048.0f, std::max(0.0f, (fConst21 + -1.0f))));
		fConst24 = std::floor(((0.256891012f * fConst0) + 0.5f));
		fConst25 = ((0.0f - (6.90775537f * fConst24)) / fConst0);
		fConst26 = std::floor(((0.0273330007f * fConst0) + 0.5f));
		iConst27 = int(std::min(16384.0f, std::max(0.0f, (fConst24 - fConst26))));
		iConst28 = int(std::min(2048.0f, std::max(0.0f, (fConst26 + -1.0f))));
		fConst29 = std::floor(((0.127837002f * fConst0) + 0.5f));
		fConst30 = ((0.0f - (6.90775537f * fConst29)) / fConst0);
		fConst31 = std::floor(((0.0316039994f * fConst0) + 0.5f));
		iConst32 = int(std::min(8192.0f, std::max(0.0f, (fConst29 - fConst31))));
		iConst33 = int(std::min(2048.0f, std::max(0.0f, (fConst31 + -1.0f))));
		fConst34 = std::floor(((0.210389003f * fConst0) + 0.5f));
		fConst35 = ((0.0f - (6.90775537f * fConst34)) / fConst0);
		fConst36 = std::floor(((0.0244210009f * fConst0) + 0.5f));
		iConst37 = int(std::min(16384.0f, std::max(0.0f, (fConst34 - fConst36))));
		iConst38 = int(std::min(2048.0f, std::max(0.0f, (fConst36 + -1.0f))));
		fConst39 = std::floor(((0.153128996f * fConst0) + 0.5f));
		fConst40 = ((0.0f - (6.90775537f * fConst39)) / fConst0);
		fConst41 = std::floor(((0.0203460008f * fConst0) + 0.5f));
		iConst42 = int(std::min(8192.0f, std::max(0.0f, (fConst39 - fConst41))));
		iConst43 = int(std::min(1024.0f, std::max(0.0f, (fConst41 + -1.0f))));

	}

	virtual void instanceResetUserInterface() {
		fVslider0 = FAUSTFLOAT(6000.0f);
		fVslider1 = FAUSTFLOAT(1.0f);
		fVslider2 = FAUSTFLOAT(1.0f);
		fVslider3 = FAUSTFLOAT(200.0f);
		fVslider4 = FAUSTFLOAT(20.0f);

	}

	virtual void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fRec11[l0] = 0.0f;

		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fRec10[l1] = 0.0f;

		}
		IOTA = 0;
		for (int l2 = 0; (l2 < 16384); l2 = (l2 + 1)) {
			fVec0[l2] = 0.0f;

		}
		for (int l3 = 0; (l3 < 16384); l3 = (l3 + 1)) {
			fVec1[l3] = 0.0f;

		}
		for (int l4 = 0; (l4 < 2048); l4 = (l4 + 1)) {
			fVec2[l4] = 0.0f;

		}
		for (int l5 = 0; (l5 < 2); l5 = (l5 + 1)) {
			fRec8[l5] = 0.0f;

		}
		for (int l6 = 0; (l6 < 2); l6 = (l6 + 1)) {
			fRec15[l6] = 0.0f;

		}
		for (int l7 = 0; (l7 < 2); l7 = (l7 + 1)) {
			fRec14[l7] = 0.0f;

		}
		for (int l8 = 0; (l8 < 32768); l8 = (l8 + 1)) {
			fVec3[l8] = 0.0f;

		}
		for (int l9 = 0; (l9 < 16384); l9 = (l9 + 1)) {
			fVec4[l9] = 0.0f;

		}
		for (int l10 = 0; (l10 < 2048); l10 = (l10 + 1)) {
			fVec5[l10] = 0.0f;

		}
		for (int l11 = 0; (l11 < 2); l11 = (l11 + 1)) {
			fRec12[l11] = 0.0f;

		}
		for (int l12 = 0; (l12 < 2); l12 = (l12 + 1)) {
			fRec19[l12] = 0.0f;

		}
		for (int l13 = 0; (l13 < 2); l13 = (l13 + 1)) {
			fRec18[l13] = 0.0f;

		}
		for (int l14 = 0; (l14 < 16384); l14 = (l14 + 1)) {
			fVec6[l14] = 0.0f;

		}
		for (int l15 = 0; (l15 < 4096); l15 = (l15 + 1)) {
			fVec7[l15] = 0.0f;

		}
		for (int l16 = 0; (l16 < 2); l16 = (l16 + 1)) {
			fRec16[l16] = 0.0f;

		}
		for (int l17 = 0; (l17 < 2); l17 = (l17 + 1)) {
			fRec23[l17] = 0.0f;

		}
		for (int l18 = 0; (l18 < 2); l18 = (l18 + 1)) {
			fRec22[l18] = 0.0f;

		}
		for (int l19 = 0; (l19 < 16384); l19 = (l19 + 1)) {
			fVec8[l19] = 0.0f;

		}
		for (int l20 = 0; (l20 < 4096); l20 = (l20 + 1)) {
			fVec9[l20] = 0.0f;

		}
		for (int l21 = 0; (l21 < 2); l21 = (l21 + 1)) {
			fRec20[l21] = 0.0f;

		}
		for (int l22 = 0; (l22 < 2); l22 = (l22 + 1)) {
			fRec27[l22] = 0.0f;

		}
		for (int l23 = 0; (l23 < 2); l23 = (l23 + 1)) {
			fRec26[l23] = 0.0f;

		}
		for (int l24 = 0; (l24 < 32768); l24 = (l24 + 1)) {
			fVec10[l24] = 0.0f;

		}
		for (int l25 = 0; (l25 < 4096); l25 = (l25 + 1)) {
			fVec11[l25] = 0.0f;

		}
		for (int l26 = 0; (l26 < 2); l26 = (l26 + 1)) {
			fRec24[l26] = 0.0f;

		}
		for (int l27 = 0; (l27 < 2); l27 = (l27 + 1)) {
			fRec31[l27] = 0.0f;

		}
		for (int l28 = 0; (l28 < 2); l28 = (l28 + 1)) {
			fRec30[l28] = 0.0f;

		}
		for (int l29 = 0; (l29 < 16384); l29 = (l29 + 1)) {
			fVec12[l29] = 0.0f;

		}
		for (int l30 = 0; (l30 < 4096); l30 = (l30 + 1)) {
			fVec13[l30] = 0.0f;

		}
		for (int l31 = 0; (l31 < 2); l31 = (l31 + 1)) {
			fRec28[l31] = 0.0f;

		}
		for (int l32 = 0; (l32 < 2); l32 = (l32 + 1)) {
			fRec35[l32] = 0.0f;

		}
		for (int l33 = 0; (l33 < 2); l33 = (l33 + 1)) {
			fRec34[l33] = 0.0f;

		}
		for (int l34 = 0; (l34 < 32768); l34 = (l34 + 1)) {
			fVec14[l34] = 0.0f;

		}
		for (int l35 = 0; (l35 < 4096); l35 = (l35 + 1)) {
			fVec15[l35] = 0.0f;

		}
		for (int l36 = 0; (l36 < 2); l36 = (l36 + 1)) {
			fRec32[l36] = 0.0f;

		}
		for (int l37 = 0; (l37 < 2); l37 = (l37 + 1)) {
			fRec39[l37] = 0.0f;

		}
		for (int l38 = 0; (l38 < 2); l38 = (l38 + 1)) {
			fRec38[l38] = 0.0f;

		}
		for (int l39 = 0; (l39 < 16384); l39 = (l39 + 1)) {
			fVec16[l39] = 0.0f;

		}
		for (int l40 = 0; (l40 < 2048); l40 = (l40 + 1)) {
			fVec17[l40] = 0.0f;

		}
		for (int l41 = 0; (l41 < 2); l41 = (l41 + 1)) {
			fRec36[l41] = 0.0f;

		}
		for (int l42 = 0; (l42 < 3); l42 = (l42 + 1)) {
			fRec0[l42] = 0.0f;

		}
		for (int l43 = 0; (l43 < 3); l43 = (l43 + 1)) {
			fRec1[l43] = 0.0f;

		}
		for (int l44 = 0; (l44 < 3); l44 = (l44 + 1)) {
			fRec2[l44] = 0.0f;

		}
		for (int l45 = 0; (l45 < 3); l45 = (l45 + 1)) {
			fRec3[l45] = 0.0f;

		}
		for (int l46 = 0; (l46 < 3); l46 = (l46 + 1)) {
			fRec4[l46] = 0.0f;

		}
		for (int l47 = 0; (l47 < 3); l47 = (l47 + 1)) {
			fRec5[l47] = 0.0f;

		}
		for (int l48 = 0; (l48 < 3); l48 = (l48 + 1)) {
			fRec6[l48] = 0.0f;

		}
		for (int l49 = 0; (l49 < 3); l49 = (l49 + 1)) {
			fRec7[l49] = 0.0f;

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

	virtual ZitaReverb_dsp* clone() {
		return new ZitaReverb_dsp();
	}
	virtual int getSampleRate() {
		return fSamplingFreq;

	}

	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("ZitaReverb");
		ui_interface->declare(&fVslider4, "1", "");
		ui_interface->declare(&fVslider4, "tooltip", "reverb pre-delay");
		ui_interface->declare(&fVslider4, "unit", "ms");
		ui_interface->addVerticalSlider("pre_del", &fVslider4, 20.0f, 0.0f, 200.0f, 1.0f);
		ui_interface->declare(&fVslider3, "2", "");
		ui_interface->declare(&fVslider3, "scale", "log");
		ui_interface->declare(&fVslider3, "tooltip", "low band cutoff frequency");
		ui_interface->declare(&fVslider3, "unit", "Hz");
		ui_interface->addVerticalSlider("lf_fc", &fVslider3, 200.0f, 30.0f, 1200.0f, 1.0f);
		ui_interface->declare(&fVslider2, "3", "");
		ui_interface->declare(&fVslider2, "tooltip", "-60db decay time for low band");
		ui_interface->declare(&fVslider2, "unit", "s");
		ui_interface->addVerticalSlider("low_rt60", &fVslider2, 1.0f, 0.100000001f, 3.0f, 0.100000001f);
		ui_interface->declare(&fVslider1, "4", "");
		ui_interface->declare(&fVslider1, "tooltip", "-60db decay time for middle band");
		ui_interface->declare(&fVslider1, "unit", "s");
		ui_interface->addVerticalSlider("mid_rt60", &fVslider1, 1.0f, 0.100000001f, 3.0f, 0.100000001f);
		ui_interface->declare(&fVslider0, "5", "");
		ui_interface->declare(&fVslider0, "scale", "log");
		ui_interface->declare(&fVslider0, "tooltip", "damping frequency (decay time is 1/2 mid)");
		ui_interface->declare(&fVslider0, "unit", "Hz");
		ui_interface->addVerticalSlider("hf_damp", &fVslider0, 6000.0f, 1200.0f, 23520.0f, 1.0f);
		ui_interface->closeBox();

	}

	virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = std::cos((fConst1 * float(fVslider0)));
		float fSlow1 = float(fVslider1);
		float fSlow2 = std::exp((fConst3 / fSlow1));
		float fSlow3 = mydsp_faustpower2_f(fSlow2);
		float fSlow4 = (1.0f - (fSlow0 * fSlow3));
		float fSlow5 = (1.0f - fSlow3);
		float fSlow6 = (fSlow4 / fSlow5);
		float fSlow7 = std::sqrt(std::max(0.0f, ((mydsp_faustpower2_f(fSlow4) / mydsp_faustpower2_f(fSlow5)) + -1.0f)));
		float fSlow8 = (fSlow6 - fSlow7);
		float fSlow9 = (fSlow2 * (fSlow7 + (1.0f - fSlow6)));
		float fSlow10 = float(fVslider2);
		float fSlow11 = ((std::exp((fConst3 / fSlow10)) / fSlow2) + -1.0f);
		float fSlow12 = (1.0f / std::tan((fConst4 * float(fVslider3))));
		float fSlow13 = (fSlow12 + 1.0f);
		float fSlow14 = (1.0f / fSlow13);
		float fSlow15 = (0.0f - ((1.0f - fSlow12) / fSlow13));
		int iSlow16 = int(std::min(8192.0f, std::max(0.0f, (fConst7 * float(fVslider4)))));
		float fSlow17 = std::exp((fConst10 / fSlow1));
		float fSlow18 = mydsp_faustpower2_f(fSlow17);
		float fSlow19 = (1.0f - (fSlow18 * fSlow0));
		float fSlow20 = (1.0f - fSlow18);
		float fSlow21 = (fSlow19 / fSlow20);
		float fSlow22 = std::sqrt(std::max(0.0f, ((mydsp_faustpower2_f(fSlow19) / mydsp_faustpower2_f(fSlow20)) + -1.0f)));
		float fSlow23 = (fSlow21 - fSlow22);
		float fSlow24 = (fSlow17 * (fSlow22 + (1.0f - fSlow21)));
		float fSlow25 = ((std::exp((fConst10 / fSlow10)) / fSlow17) + -1.0f);
		float fSlow26 = std::exp((fConst15 / fSlow1));
		float fSlow27 = mydsp_faustpower2_f(fSlow26);
		float fSlow28 = (1.0f - (fSlow0 * fSlow27));
		float fSlow29 = (1.0f - fSlow27);
		float fSlow30 = (fSlow28 / fSlow29);
		float fSlow31 = std::sqrt(std::max(0.0f, ((mydsp_faustpower2_f(fSlow28) / mydsp_faustpower2_f(fSlow29)) + -1.0f)));
		float fSlow32 = (fSlow30 - fSlow31);
		float fSlow33 = (fSlow26 * (fSlow31 + (1.0f - fSlow30)));
		float fSlow34 = ((std::exp((fConst15 / fSlow10)) / fSlow26) + -1.0f);
		float fSlow35 = std::exp((fConst20 / fSlow1));
		float fSlow36 = mydsp_faustpower2_f(fSlow35);
		float fSlow37 = (1.0f - (fSlow0 * fSlow36));
		float fSlow38 = (1.0f - fSlow36);
		float fSlow39 = (fSlow37 / fSlow38);
		float fSlow40 = std::sqrt(std::max(0.0f, ((mydsp_faustpower2_f(fSlow37) / mydsp_faustpower2_f(fSlow38)) + -1.0f)));
		float fSlow41 = (fSlow39 - fSlow40);
		float fSlow42 = (fSlow35 * (fSlow40 + (1.0f - fSlow39)));
		float fSlow43 = ((std::exp((fConst20 / fSlow10)) / fSlow35) + -1.0f);
		float fSlow44 = std::exp((fConst25 / fSlow1));
		float fSlow45 = mydsp_faustpower2_f(fSlow44);
		float fSlow46 = (1.0f - (fSlow0 * fSlow45));
		float fSlow47 = (1.0f - fSlow45);
		float fSlow48 = (fSlow46 / fSlow47);
		float fSlow49 = std::sqrt(std::max(0.0f, ((mydsp_faustpower2_f(fSlow46) / mydsp_faustpower2_f(fSlow47)) + -1.0f)));
		float fSlow50 = (fSlow48 - fSlow49);
		float fSlow51 = (fSlow44 * (fSlow49 + (1.0f - fSlow48)));
		float fSlow52 = ((std::exp((fConst25 / fSlow10)) / fSlow44) + -1.0f);
		float fSlow53 = std::exp((fConst30 / fSlow1));
		float fSlow54 = mydsp_faustpower2_f(fSlow53);
		float fSlow55 = (1.0f - (fSlow0 * fSlow54));
		float fSlow56 = (1.0f - fSlow54);
		float fSlow57 = (fSlow55 / fSlow56);
		float fSlow58 = std::sqrt(std::max(0.0f, ((mydsp_faustpower2_f(fSlow55) / mydsp_faustpower2_f(fSlow56)) + -1.0f)));
		float fSlow59 = (fSlow57 - fSlow58);
		float fSlow60 = (fSlow53 * (fSlow58 + (1.0f - fSlow57)));
		float fSlow61 = ((std::exp((fConst30 / fSlow10)) / fSlow53) + -1.0f);
		float fSlow62 = std::exp((fConst35 / fSlow1));
		float fSlow63 = mydsp_faustpower2_f(fSlow62);
		float fSlow64 = (1.0f - (fSlow0 * fSlow63));
		float fSlow65 = (1.0f - fSlow63);
		float fSlow66 = (fSlow64 / fSlow65);
		float fSlow67 = std::sqrt(std::max(0.0f, ((mydsp_faustpower2_f(fSlow64) / mydsp_faustpower2_f(fSlow65)) + -1.0f)));
		float fSlow68 = (fSlow66 - fSlow67);
		float fSlow69 = (fSlow62 * (fSlow67 + (1.0f - fSlow66)));
		float fSlow70 = ((std::exp((fConst35 / fSlow10)) / fSlow62) + -1.0f);
		float fSlow71 = std::exp((fConst40 / fSlow1));
		float fSlow72 = mydsp_faustpower2_f(fSlow71);
		float fSlow73 = (1.0f - (fSlow0 * fSlow72));
		float fSlow74 = (1.0f - fSlow72);
		float fSlow75 = (fSlow73 / fSlow74);
		float fSlow76 = std::sqrt(std::max(0.0f, ((mydsp_faustpower2_f(fSlow73) / mydsp_faustpower2_f(fSlow74)) + -1.0f)));
		float fSlow77 = (fSlow75 - fSlow76);
		float fSlow78 = (fSlow71 * (fSlow76 + (1.0f - fSlow75)));
		float fSlow79 = ((std::exp((fConst40 / fSlow10)) / fSlow71) + -1.0f);
		for (int i = 0; (i < count); i = (i + 1)) {
			fRec11[0] = ((fSlow14 * (fRec6[1] + fRec6[2])) + (fSlow15 * fRec11[1]));
			fRec10[0] = ((fSlow8 * fRec10[1]) + (fSlow9 * (fRec6[1] + (fSlow11 * fRec11[0]))));
			fVec0[(IOTA & 16383)] = ((0.353553385f * fRec10[0]) + 9.99999968e-21f);
			fVec1[(IOTA & 16383)] = float(input0[i]);
			float fTemp0 = (0.300000012f * fVec1[((IOTA - iSlow16) & 16383)]);
			float fTemp1 = (fVec0[((IOTA - iConst6) & 16383)] - ((0.600000024f * fRec8[1]) + fTemp0));
			fVec2[(IOTA & 2047)] = fTemp1;
			fRec8[0] = fVec2[((IOTA - iConst8) & 2047)];
			float fRec9 = (0.600000024f * fTemp1);
			fRec15[0] = ((fSlow14 * (fRec7[1] + fRec7[2])) + (fSlow15 * fRec15[1]));
			fRec14[0] = ((fSlow23 * fRec14[1]) + (fSlow24 * (fRec7[1] + (fSlow25 * fRec15[0]))));
			fVec3[(IOTA & 32767)] = ((0.353553385f * fRec14[0]) + 9.99999968e-21f);
			fVec4[(IOTA & 16383)] = float(input1[i]);
			float fTemp2 = (0.300000012f * fVec4[((IOTA - iSlow16) & 16383)]);
			float fTemp3 = (((0.600000024f * fRec12[1]) + fVec3[((IOTA - iConst12) & 32767)]) - fTemp2);
			fVec5[(IOTA & 2047)] = fTemp3;
			fRec12[0] = fVec5[((IOTA - iConst13) & 2047)];
			float fRec13 = (0.0f - (0.600000024f * fTemp3));
			fRec19[0] = ((fSlow14 * (fRec5[1] + fRec5[2])) + (fSlow15 * fRec19[1]));
			fRec18[0] = ((fSlow32 * fRec18[1]) + (fSlow33 * (fRec5[1] + (fSlow34 * fRec19[0]))));
			fVec6[(IOTA & 16383)] = ((0.353553385f * fRec18[0]) + 9.99999968e-21f);
			float fTemp4 = (((0.600000024f * fRec16[1]) + fVec6[((IOTA - iConst17) & 16383)]) + fTemp2);
			fVec7[(IOTA & 4095)] = fTemp4;
			fRec16[0] = fVec7[((IOTA - iConst18) & 4095)];
			float fRec17 = (0.0f - (0.600000024f * fTemp4));
			fRec23[0] = ((fSlow14 * (fRec4[1] + fRec4[2])) + (fSlow15 * fRec23[1]));
			fRec22[0] = ((fSlow41 * fRec22[1]) + (fSlow42 * (fRec4[1] + (fSlow43 * fRec23[0]))));
			fVec8[(IOTA & 16383)] = ((0.353553385f * fRec22[0]) + 9.99999968e-21f);
			float fTemp5 = ((fVec8[((IOTA - iConst22) & 16383)] + fTemp0) - (0.600000024f * fRec20[1]));
			fVec9[(IOTA & 4095)] = fTemp5;
			fRec20[0] = fVec9[((IOTA - iConst23) & 4095)];
			float fRec21 = (0.600000024f * fTemp5);
			fRec27[0] = ((fSlow14 * (fRec3[1] + fRec3[2])) + (fSlow15 * fRec27[1]));
			fRec26[0] = ((fSlow50 * fRec26[1]) + (fSlow51 * (fRec3[1] + (fSlow52 * fRec27[0]))));
			fVec10[(IOTA & 32767)] = ((0.353553385f * fRec26[0]) + 9.99999968e-21f);
			float fTemp6 = (((0.600000024f * fRec24[1]) + fVec10[((IOTA - iConst27) & 32767)]) - fTemp2);
			fVec11[(IOTA & 4095)] = fTemp6;
			fRec24[0] = fVec11[((IOTA - iConst28) & 4095)];
			float fRec25 = (0.0f - (0.600000024f * fTemp6));
			fRec31[0] = ((fSlow14 * (fRec2[1] + fRec2[2])) + (fSlow15 * fRec31[1]));
			fRec30[0] = ((fSlow59 * fRec30[1]) + (fSlow60 * (fRec2[1] + (fSlow61 * fRec31[0]))));
			fVec12[(IOTA & 16383)] = ((0.353553385f * fRec30[0]) + 9.99999968e-21f);
			float fTemp7 = (fVec12[((IOTA - iConst32) & 16383)] - ((0.600000024f * fRec28[1]) + fTemp0));
			fVec13[(IOTA & 4095)] = fTemp7;
			fRec28[0] = fVec13[((IOTA - iConst33) & 4095)];
			float fRec29 = (0.600000024f * fTemp7);
			fRec35[0] = ((fSlow14 * (fRec1[1] + fRec1[2])) + (fSlow15 * fRec35[1]));
			fRec34[0] = ((fSlow68 * fRec34[1]) + (fSlow69 * (fRec1[1] + (fSlow70 * fRec35[0]))));
			fVec14[(IOTA & 32767)] = ((0.353553385f * fRec34[0]) + 9.99999968e-21f);
			float fTemp8 = (((0.600000024f * fRec32[1]) + fVec14[((IOTA - iConst37) & 32767)]) + fTemp2);
			fVec15[(IOTA & 4095)] = fTemp8;
			fRec32[0] = fVec15[((IOTA - iConst38) & 4095)];
			float fRec33 = (0.0f - (0.600000024f * fTemp8));
			fRec39[0] = ((fSlow14 * (fRec0[1] + fRec0[2])) + (fSlow15 * fRec39[1]));
			fRec38[0] = ((fSlow77 * fRec38[1]) + (fSlow78 * (fRec0[1] + (fSlow79 * fRec39[0]))));
			fVec16[(IOTA & 16383)] = ((0.353553385f * fRec38[0]) + 9.99999968e-21f);
			float fTemp9 = ((fVec16[((IOTA - iConst42) & 16383)] + fTemp0) - (0.600000024f * fRec36[1]));
			fVec17[(IOTA & 2047)] = fTemp9;
			fRec36[0] = fVec17[((IOTA - iConst43) & 2047)];
			float fRec37 = (0.600000024f * fTemp9);
			float fTemp10 = (fRec36[1] + fRec37);
			float fTemp11 = (fRec33 + (fRec32[1] + fTemp10));
			float fTemp12 = (fRec25 + (fRec24[1] + (fRec29 + (fRec28[1] + fTemp11))));
			fRec0[0] = (fRec9 + (fRec13 + (fRec12[1] + (fRec8[1] + (fRec17 + (fRec16[1] + (fRec21 + (fRec20[1] + fTemp12))))))));
			float fTemp13 = (fRec29 + (fRec28[1] + fTemp10));
			float fTemp14 = (fRec32[1] + fRec33);
			float fTemp15 = (fRec25 + (fRec24[1] + fTemp14));
			fRec1[0] = ((fRec9 + (fRec8[1] + (fRec21 + (fRec20[1] + fTemp13)))) - (fRec13 + (fRec12[1] + (fRec17 + (fRec16[1] + fTemp15)))));
			float fTemp16 = (fRec25 + (fRec24[1] + (fRec28[1] + fRec29)));
			fRec2[0] = ((fRec17 + (fRec16[1] + (fRec21 + (fRec20[1] + fTemp11)))) - (fRec9 + (fRec13 + (fRec12[1] + (fRec8[1] + fTemp16)))));
			float fTemp17 = (fRec25 + (fRec24[1] + fTemp10));
			float fTemp18 = (fRec29 + (fRec28[1] + fTemp14));
			fRec3[0] = ((fRec13 + (fRec12[1] + (fRec21 + (fRec20[1] + fTemp17)))) - (fRec9 + (fRec8[1] + (fRec17 + (fRec16[1] + fTemp18)))));
			fRec4[0] = (fTemp12 - (fRec9 + (fRec13 + (fRec12[1] + (fRec8[1] + (fRec17 + (fRec16[1] + (fRec20[1] + fRec21))))))));
			fRec5[0] = ((fRec13 + (fRec12[1] + (fRec17 + (fRec16[1] + fTemp13)))) - (fRec9 + (fRec8[1] + (fRec21 + (fRec20[1] + fTemp15)))));
			fRec6[0] = ((fRec9 + (fRec13 + (fRec12[1] + (fRec8[1] + fTemp11)))) - (fRec17 + (fRec16[1] + (fRec21 + (fRec20[1] + fTemp16)))));
			fRec7[0] = ((fRec9 + (fRec8[1] + (fRec17 + (fRec16[1] + fTemp17)))) - (fRec13 + (fRec12[1] + (fRec21 + (fRec20[1] + fTemp18)))));
			output0[i] = FAUSTFLOAT((0.370000005f * (fRec1[0] + fRec2[0])));
			output1[i] = FAUSTFLOAT((0.370000005f * (fRec1[0] - fRec2[0])));
			fRec11[1] = fRec11[0];
			fRec10[1] = fRec10[0];
			IOTA = (IOTA + 1);
			fRec8[1] = fRec8[0];
			fRec15[1] = fRec15[0];
			fRec14[1] = fRec14[0];
			fRec12[1] = fRec12[0];
			fRec19[1] = fRec19[0];
			fRec18[1] = fRec18[0];
			fRec16[1] = fRec16[0];
			fRec23[1] = fRec23[0];
			fRec22[1] = fRec22[0];
			fRec20[1] = fRec20[0];
			fRec27[1] = fRec27[0];
			fRec26[1] = fRec26[0];
			fRec24[1] = fRec24[0];
			fRec31[1] = fRec31[0];
			fRec30[1] = fRec30[0];
			fRec28[1] = fRec28[0];
			fRec35[1] = fRec35[0];
			fRec34[1] = fRec34[0];
			fRec32[1] = fRec32[0];
			fRec39[1] = fRec39[0];
			fRec38[1] = fRec38[0];
			fRec36[1] = fRec36[0];
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
			fRec1[2] = fRec1[1];
			fRec1[1] = fRec1[0];
			fRec2[2] = fRec2[1];
			fRec2[1] = fRec2[0];
			fRec3[2] = fRec3[1];
			fRec3[1] = fRec3[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec5[2] = fRec5[1];
			fRec5[1] = fRec5[0];
			fRec6[2] = fRec6[1];
			fRec6[1] = fRec6[0];
			fRec7[2] = fRec7[1];
			fRec7[1] = fRec7[0];

		}

	}


};

class ZitaReverb
{
  private:

	ZitaReverb_dsp _dsp;
    APIUI ui;

  public:        
    ZitaReverb() {
        _dsp.buildUserInterface(&ui);
    }

    void init(double sampleRate)
    {
		ZitaReverb_dsp::classInit(int(sampleRate));
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
