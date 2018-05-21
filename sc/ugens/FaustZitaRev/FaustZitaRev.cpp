/* ------------------------------------------------------------
author: "JOS, Revised by RM"
name: "zitaRev"
version: "0.0"
Code generated with Faust 2.5.15 (https://faust.grame.fr)
Compilation options: cpp, -scal -ftz 0
------------------------------------------------------------ */

#ifndef  __mydsp_H__
#define  __mydsp_H__

//-------------------------------------------------------------------
// FAUST architecture file for SuperCollider.
// Copyright (C) 2005-2012 Stefan Kersten.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
// 02111-1307 USA
//-------------------------------------------------------------------

// If other than 'faust2sc --prefix Faust' is used, sed this as well:
#if !defined(SC_FAUST_PREFIX)
#define SC_FAUST_PREFIX "Faust"
#endif

#include <map>
#include <string>
#include <string.h>
#include <SC_PlugIn.h>

/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2017 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __dsp__
#define __dsp__

#include <string>

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

class UI;
struct Meta;

/**
 * DSP memory manager.
 */

struct dsp_memory_manager {
    
    virtual ~dsp_memory_manager() {}
    
    virtual void* allocate(size_t size) = 0;
    virtual void destroy(void* ptr) = 0;
    
};

/**
* Signal processor definition.
*/

class dsp {

    public:

        dsp() {}
        virtual ~dsp() {}

        /* Return instance number of audio inputs */
        virtual int getNumInputs() = 0;
    
        /* Return instance number of audio outputs */
        virtual int getNumOutputs() = 0;
    
        /**
         * Trigger the ui_interface parameter with instance specific calls
         * to 'addBtton', 'addVerticalSlider'... in order to build the UI.
         *
         * @param ui_interface - the user interface builder
         */
        virtual void buildUserInterface(UI* ui_interface) = 0;
    
        /* Returns the sample rate currently used by the instance */
        virtual int getSampleRate() = 0;
    
        /**
         * Global init, calls the following methods:
         * - static class 'classInit': static tables initialization
         * - 'instanceInit': constants and instance state initialization
         *
         * @param samplingRate - the sampling rate in Hertz
         */
        virtual void init(int samplingRate) = 0;

        /**
         * Init instance state
         *
         * @param samplingRate - the sampling rate in Hertz
         */
        virtual void instanceInit(int samplingRate) = 0;

        /**
         * Init instance constant state
         *
         * @param samplingRate - the sampling rate in Hertz
         */
        virtual void instanceConstants(int samplingRate) = 0;
    
        /* Init default control parameters values */
        virtual void instanceResetUserInterface() = 0;
    
        /* Init instance state (delay lines...) */
        virtual void instanceClear() = 0;
 
        /**
         * Return a clone of the instance.
         *
         * @return a copy of the instance on success, otherwise a null pointer.
         */
        virtual dsp* clone() = 0;
    
        /**
         * Trigger the Meta* parameter with instance specific calls to 'declare' (key, value) metadata.
         *
         * @param m - the Meta* meta user
         */
        virtual void metadata(Meta* m) = 0;
    
        /**
         * DSP instance computation, to be called with successive in/out audio buffers.
         *
         * @param count - the number of frames to compute
         * @param inputs - the input audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         * @param outputs - the output audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         *
         */
        virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) = 0;
    
        /**
         * DSP instance computation: alternative method to be used by subclasses.
         *
         * @param date_usec - the timestamp in microsec given by audio driver.
         * @param count - the number of frames to compute
         * @param inputs - the input audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         * @param outputs - the output audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         *
         */
        virtual void compute(double date_usec, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) { compute(count, inputs, outputs); }
       
};

/**
 * Generic DSP decorator.
 */

class decorator_dsp : public dsp {

    protected:

        dsp* fDSP;

    public:

        decorator_dsp(dsp* dsp = 0):fDSP(dsp) {}
        virtual ~decorator_dsp() { delete fDSP; }

        virtual int getNumInputs() { return fDSP->getNumInputs(); }
        virtual int getNumOutputs() { return fDSP->getNumOutputs(); }
        virtual void buildUserInterface(UI* ui_interface) { fDSP->buildUserInterface(ui_interface); }
        virtual int getSampleRate() { return fDSP->getSampleRate(); }
        virtual void init(int samplingRate) { fDSP->init(samplingRate); }
        virtual void instanceInit(int samplingRate) { fDSP->instanceInit(samplingRate); }
        virtual void instanceConstants(int samplingRate) { fDSP->instanceConstants(samplingRate); }
        virtual void instanceResetUserInterface() { fDSP->instanceResetUserInterface(); }
        virtual void instanceClear() { fDSP->instanceClear(); }
        virtual decorator_dsp* clone() { return new decorator_dsp(fDSP->clone()); }
        virtual void metadata(Meta* m) { fDSP->metadata(m); }
        // Beware: subclasses usually have to overload the two 'compute' methods
        virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) { fDSP->compute(count, inputs, outputs); }
        virtual void compute(double date_usec, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) { fDSP->compute(date_usec, count, inputs, outputs); }
    
};

/**
 * DSP factory class.
 */

class dsp_factory {
    
    protected:
    
        // So that to force sub-classes to use deleteDSPFactory(dsp_factory* factory);
        virtual ~dsp_factory() {}
    
    public:
    
        virtual std::string getName() = 0;
        virtual std::string getSHAKey() = 0;
        virtual std::string getDSPCode() = 0;
    
        virtual dsp* createDSPInstance() = 0;
    
        virtual void setMemoryManager(dsp_memory_manager* manager) = 0;
        virtual dsp_memory_manager* getMemoryManager() = 0;
    
};

/**
 * On Intel set FZ (Flush to Zero) and DAZ (Denormals Are Zero)
 * flags to avoid costly denormals.
 */

#ifdef __SSE__
    #include <xmmintrin.h>
    #ifdef __SSE2__
        #define AVOIDDENORMALS _mm_setcsr(_mm_getcsr() | 0x8040)
    #else
        #define AVOIDDENORMALS _mm_setcsr(_mm_getcsr() | 0x8000)
    #endif
#else
    #define AVOIDDENORMALS
#endif

#endif
/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2017 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __UI_H__
#define __UI_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

/*******************************************************************************
 * UI : Faust DSP User Interface
 * User Interface as expected by the buildUserInterface() method of a DSP.
 * This abstract class contains only the method that the Faust compiler can
 * generate to describe a DSP user interface.
 ******************************************************************************/

struct Soundfile;

class UI
{

    public:

        UI() {}

        virtual ~UI() {}

        // -- widget's layouts

        virtual void openTabBox(const char* label) = 0;
        virtual void openHorizontalBox(const char* label) = 0;
        virtual void openVerticalBox(const char* label) = 0;
        virtual void closeBox() = 0;

        // -- active widgets

        virtual void addButton(const char* label, FAUSTFLOAT* zone) = 0;
        virtual void addCheckButton(const char* label, FAUSTFLOAT* zone) = 0;
        virtual void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
        virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
        virtual void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;

        // -- passive widgets

        virtual void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) = 0;
        virtual void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) = 0;
    
        // -- soundfiles
    
        virtual void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) = 0;

        // -- metadata declarations

        virtual void declare(FAUSTFLOAT*, const char*, const char*) {}
};

#endif
/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2017 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/
 
#ifndef __misc__
#define __misc__

#include <algorithm>
#include <map>
#include <string.h>
#include <stdlib.h>

/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2017 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __meta__
#define __meta__

struct Meta
{
    virtual void declare(const char* key, const char* value) = 0;
    virtual ~Meta() {};
};

#endif

using std::max;
using std::min;

struct XXXX_Meta : std::map<const char*, const char*>
{
    void declare(const char* key, const char* value) { (*this)[key]=value; }
};

struct MY_Meta : Meta, std::map<const char*, const char*>
{
    void declare(const char* key, const char* value) { (*this)[key]=value; }
};

inline int lsr(int x, int n)	{ return int(((unsigned int)x) >> n); }

inline int int2pow2(int x)		{ int r = 0; while ((1<<r) < x) r++; return r; }

inline long lopt(char* argv[], const char* name, long def)
{
	int	i;
	for (i = 0; argv[i]; i++) if (!strcmp(argv[i], name)) return atoi(argv[i+1]);
	return def;
}

inline bool isopt(char* argv[], const char* name)
{
	int	i;
	for (i = 0; argv[i]; i++) if (!strcmp(argv[i], name)) return true;
	return false;
}

inline const char* lopts(char* argv[], const char* name, const char* def)
{
	int	i;
	for (i = 0; argv[i]; i++) if (!strcmp(argv[i], name)) return argv[i+1];
	return def;
}

#endif


using namespace std;

#if defined(__GNUC__) && __GNUC__ >= 4
    #define FAUST_EXPORT __attribute__((visibility("default")))
#else
    #define FAUST_EXPORT  SC_API_EXPORT
#endif

#ifdef WIN32
    #define STRDUP _strdup
#else
    #define STRDUP strdup
#endif

//----------------------------------------------------------------------------
// Vector intrinsics
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Metadata
//----------------------------------------------------------------------------

class MetaData : public Meta
               , public std::map<std::string, std::string>
{
public:
    void declare(const char* key, const char* value)
    {
        (*this)[key] = value;
    }
};

//----------------------------------------------------------------------------
// Control counter
//----------------------------------------------------------------------------

class ControlCounter : public UI
{
public:
    ControlCounter()
        : mNumControlInputs(0),
          mNumControlOutputs(0)
    {}

    size_t getNumControls() const { return getNumControlInputs(); }
    size_t getNumControlInputs() const { return mNumControlInputs; }
    size_t getNumControlOutputs() const { return mNumControlOutputs; }

    // Layout widgets
    virtual void openTabBox(const char* label) { }
    virtual void openHorizontalBox(const char* label) { }
    virtual void openVerticalBox(const char* label) { }
    virtual void closeBox() { }

    // Active widgets
    virtual void addButton(const char* label, FAUSTFLOAT* zone)
    { addControlInput(); }
    virtual void addCheckButton(const char* label, FAUSTFLOAT* zone)
    { addControlInput(); }
    virtual void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    { addControlInput(); }
    virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    { addControlInput(); }
    virtual void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    { addControlInput(); }

    // Passive widgets
    virtual void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
    { addControlOutput(); }
    virtual void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
    { addControlOutput(); }
    
    virtual void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) {}

protected:
    void addControlInput() { mNumControlInputs++; }
    void addControlOutput() { mNumControlOutputs++; }

private:
    size_t mNumControlInputs;
    size_t mNumControlOutputs;
};

//----------------------------------------------------------------------------
// UI control
//----------------------------------------------------------------------------

struct Control
{
    typedef void (*UpdateFunction)(Control* self, FAUSTFLOAT value);

    UpdateFunction updateFunction;
    FAUSTFLOAT* zone;
    FAUSTFLOAT min, max;

    inline void update(FAUSTFLOAT value)
    {
        (*updateFunction)(this, value);
    }

    static void simpleUpdate(Control* self, FAUSTFLOAT value)
    {
        *self->zone = value;
    }
    static void boundedUpdate(Control* self, FAUSTFLOAT value)
    {
        *self->zone = sc_clip(value, self->min, self->max);
    }
};

//----------------------------------------------------------------------------
// Control allocator
//----------------------------------------------------------------------------

class ControlAllocator : public UI
{
public:
    ControlAllocator(Control* controls)
        : mControls(controls)
    { }

    // Layout widgets
    virtual void openTabBox(const char* label) { }
    virtual void openHorizontalBox(const char* label) { }
    virtual void openVerticalBox(const char* label) { }
    virtual void closeBox() { }

    // Active widgets
    virtual void addButton(const char* label, FAUSTFLOAT* zone)
    { addSimpleControl(zone); }
    virtual void addCheckButton(const char* label, FAUSTFLOAT* zone)
    { addSimpleControl(zone); }
    virtual void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    { addBoundedControl(zone, min, max, step); }
    virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    { addBoundedControl(zone, min, max, step); }
    virtual void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    { addBoundedControl(zone, min, max, step); }

    // Passive widgets
    virtual void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) {}
    virtual void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) {}
    virtual void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) {}

private:
    void addControl(Control::UpdateFunction updateFunction, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT /* step */)
    {
        Control* ctrl        = mControls++;
        ctrl->updateFunction = updateFunction;
        ctrl->zone           = zone;
        ctrl->min            = min;
        ctrl->max            = max;
    }
    void addSimpleControl(FAUSTFLOAT* zone)
    {
        addControl(Control::simpleUpdate, zone, 0.f, 0.f, 0.f);
    }
    void addBoundedControl(FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    {
        addControl(Control::boundedUpdate, zone, min, max, step);
    }

private:
    Control* mControls;
};

//----------------------------------------------------------------------------
// FAUST generated code
//----------------------------------------------------------------------------

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <cmath>
#include <math.h>

float mydsp_faustpower2_f(float value) {
	return (value * value);
	
}

#ifndef FAUSTCLASS 
#define FAUSTCLASS mydsp
#endif
#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class mydsp : public dsp {
	
 private:
	
	FAUSTFLOAT fVslider0;
	float fRec0[2];
	FAUSTFLOAT fVslider1;
	float fRec1[2];
	int IOTA;
	float fVec0[16384];
	int fSamplingFreq;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fVslider2;
	FAUSTFLOAT fVslider3;
	FAUSTFLOAT fVslider4;
	FAUSTFLOAT fVslider5;
	float fConst2;
	float fConst3;
	FAUSTFLOAT fVslider6;
	float fConst4;
	FAUSTFLOAT fVslider7;
	FAUSTFLOAT fVslider8;
	float fConst5;
	FAUSTFLOAT fVslider9;
	float fRec15[2];
	float fRec14[2];
	float fVec1[32768];
	float fConst6;
	int iConst7;
	float fVec2[16384];
	float fConst8;
	FAUSTFLOAT fVslider10;
	float fVec3[2048];
	int iConst9;
	float fRec12[2];
	float fConst10;
	float fConst11;
	float fRec19[2];
	float fRec18[2];
	float fVec4[32768];
	float fConst12;
	int iConst13;
	float fVec5[4096];
	int iConst14;
	float fRec16[2];
	float fConst15;
	float fConst16;
	float fRec23[2];
	float fRec22[2];
	float fVec6[16384];
	float fConst17;
	int iConst18;
	float fVec7[4096];
	int iConst19;
	float fRec20[2];
	float fConst20;
	float fConst21;
	float fRec27[2];
	float fRec26[2];
	float fVec8[32768];
	float fConst22;
	int iConst23;
	float fVec9[4096];
	int iConst24;
	float fRec24[2];
	float fConst25;
	float fConst26;
	float fRec31[2];
	float fRec30[2];
	float fVec10[16384];
	float fConst27;
	int iConst28;
	float fVec11[2048];
	int iConst29;
	float fRec28[2];
	float fConst30;
	float fConst31;
	float fRec35[2];
	float fRec34[2];
	float fVec12[16384];
	float fConst32;
	int iConst33;
	float fVec13[4096];
	int iConst34;
	float fRec32[2];
	float fConst35;
	float fConst36;
	float fRec39[2];
	float fRec38[2];
	float fVec14[16384];
	float fConst37;
	int iConst38;
	float fVec15[4096];
	int iConst39;
	float fRec36[2];
	float fConst40;
	float fConst41;
	float fRec43[2];
	float fRec42[2];
	float fVec16[16384];
	float fConst42;
	int iConst43;
	float fVec17[2048];
	int iConst44;
	float fRec40[2];
	float fRec4[3];
	float fRec5[3];
	float fRec6[3];
	float fRec7[3];
	float fRec8[3];
	float fRec9[3];
	float fRec10[3];
	float fRec11[3];
	float fRec3[3];
	float fRec2[3];
	float fRec45[3];
	float fRec44[3];
	
 public:
	
	void metadata(Meta* m) { 
		m->declare("author", "JOS, Revised by RM");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "0.0");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "0.0");
		m->declare("description", "Example GUI for `zita_rev1_stereo` (mostly following the Linux `zita-rev1` GUI).");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/version", "0.0");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.1");
		m->declare("name", "zitaRev");
		m->declare("reverbs.lib/name", "Faust Reverb Library");
		m->declare("reverbs.lib/version", "0.0");
		m->declare("routes.lib/name", "Faust Signal Routing Library");
		m->declare("routes.lib/version", "0.0");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/version", "0.0");
		m->declare("version", "0.0");
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
		
	}
	
	virtual void instanceConstants(int samplingFreq) {
		fSamplingFreq = samplingFreq;
		fConst0 = min(192000.0f, max(1.0f, float(fSamplingFreq)));
		fConst1 = (6.28318548f / fConst0);
		fConst2 = floorf(((0.219990999f * fConst0) + 0.5f));
		fConst3 = ((0.0f - (6.90775537f * fConst2)) / fConst0);
		fConst4 = (6.28318548f / fConst0);
		fConst5 = (3.14159274f / fConst0);
		fConst6 = floorf(((0.0191229992f * fConst0) + 0.5f));
		iConst7 = int(min(16384.0f, max(0.0f, (fConst2 - fConst6))));
		fConst8 = (0.00100000005f * fConst0);
		iConst9 = int(min(1024.0f, max(0.0f, (fConst6 + -1.0f))));
		fConst10 = floorf(((0.256891012f * fConst0) + 0.5f));
		fConst11 = ((0.0f - (6.90775537f * fConst10)) / fConst0);
		fConst12 = floorf(((0.0273330007f * fConst0) + 0.5f));
		iConst13 = int(min(16384.0f, max(0.0f, (fConst10 - fConst12))));
		iConst14 = int(min(2048.0f, max(0.0f, (fConst12 + -1.0f))));
		fConst15 = floorf(((0.192303002f * fConst0) + 0.5f));
		fConst16 = ((0.0f - (6.90775537f * fConst15)) / fConst0);
		fConst17 = floorf(((0.0292910002f * fConst0) + 0.5f));
		iConst18 = int(min(8192.0f, max(0.0f, (fConst15 - fConst17))));
		iConst19 = int(min(2048.0f, max(0.0f, (fConst17 + -1.0f))));
		fConst20 = floorf(((0.210389003f * fConst0) + 0.5f));
		fConst21 = ((0.0f - (6.90775537f * fConst20)) / fConst0);
		fConst22 = floorf(((0.0244210009f * fConst0) + 0.5f));
		iConst23 = int(min(16384.0f, max(0.0f, (fConst20 - fConst22))));
		iConst24 = int(min(2048.0f, max(0.0f, (fConst22 + -1.0f))));
		fConst25 = floorf(((0.125f * fConst0) + 0.5f));
		fConst26 = ((0.0f - (6.90775537f * fConst25)) / fConst0);
		fConst27 = floorf(((0.0134579996f * fConst0) + 0.5f));
		iConst28 = int(min(8192.0f, max(0.0f, (fConst25 - fConst27))));
		iConst29 = int(min(1024.0f, max(0.0f, (fConst27 + -1.0f))));
		fConst30 = floorf(((0.127837002f * fConst0) + 0.5f));
		fConst31 = ((0.0f - (6.90775537f * fConst30)) / fConst0);
		fConst32 = floorf(((0.0316039994f * fConst0) + 0.5f));
		iConst33 = int(min(8192.0f, max(0.0f, (fConst30 - fConst32))));
		iConst34 = int(min(2048.0f, max(0.0f, (fConst32 + -1.0f))));
		fConst35 = floorf(((0.174713001f * fConst0) + 0.5f));
		fConst36 = ((0.0f - (6.90775537f * fConst35)) / fConst0);
		fConst37 = floorf(((0.0229039993f * fConst0) + 0.5f));
		iConst38 = int(min(8192.0f, max(0.0f, (fConst35 - fConst37))));
		iConst39 = int(min(2048.0f, max(0.0f, (fConst37 + -1.0f))));
		fConst40 = floorf(((0.153128996f * fConst0) + 0.5f));
		fConst41 = ((0.0f - (6.90775537f * fConst40)) / fConst0);
		fConst42 = floorf(((0.0203460008f * fConst0) + 0.5f));
		iConst43 = int(min(8192.0f, max(0.0f, (fConst40 - fConst42))));
		iConst44 = int(min(1024.0f, max(0.0f, (fConst42 + -1.0f))));
		
	}
	
	virtual void instanceResetUserInterface() {
		fVslider0 = FAUSTFLOAT(-20.0f);
		fVslider1 = FAUSTFLOAT(0.0f);
		fVslider2 = FAUSTFLOAT(1500.0f);
		fVslider3 = FAUSTFLOAT(0.0f);
		fVslider4 = FAUSTFLOAT(315.0f);
		fVslider5 = FAUSTFLOAT(0.0f);
		fVslider6 = FAUSTFLOAT(2.0f);
		fVslider7 = FAUSTFLOAT(6000.0f);
		fVslider8 = FAUSTFLOAT(3.0f);
		fVslider9 = FAUSTFLOAT(200.0f);
		fVslider10 = FAUSTFLOAT(60.0f);
		
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fRec0[l0] = 0.0f;
			
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fRec1[l1] = 0.0f;
			
		}
		IOTA = 0;
		for (int l2 = 0; (l2 < 16384); l2 = (l2 + 1)) {
			fVec0[l2] = 0.0f;
			
		}
		for (int l3 = 0; (l3 < 2); l3 = (l3 + 1)) {
			fRec15[l3] = 0.0f;
			
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			fRec14[l4] = 0.0f;
			
		}
		for (int l5 = 0; (l5 < 32768); l5 = (l5 + 1)) {
			fVec1[l5] = 0.0f;
			
		}
		for (int l6 = 0; (l6 < 16384); l6 = (l6 + 1)) {
			fVec2[l6] = 0.0f;
			
		}
		for (int l7 = 0; (l7 < 2048); l7 = (l7 + 1)) {
			fVec3[l7] = 0.0f;
			
		}
		for (int l8 = 0; (l8 < 2); l8 = (l8 + 1)) {
			fRec12[l8] = 0.0f;
			
		}
		for (int l9 = 0; (l9 < 2); l9 = (l9 + 1)) {
			fRec19[l9] = 0.0f;
			
		}
		for (int l10 = 0; (l10 < 2); l10 = (l10 + 1)) {
			fRec18[l10] = 0.0f;
			
		}
		for (int l11 = 0; (l11 < 32768); l11 = (l11 + 1)) {
			fVec4[l11] = 0.0f;
			
		}
		for (int l12 = 0; (l12 < 4096); l12 = (l12 + 1)) {
			fVec5[l12] = 0.0f;
			
		}
		for (int l13 = 0; (l13 < 2); l13 = (l13 + 1)) {
			fRec16[l13] = 0.0f;
			
		}
		for (int l14 = 0; (l14 < 2); l14 = (l14 + 1)) {
			fRec23[l14] = 0.0f;
			
		}
		for (int l15 = 0; (l15 < 2); l15 = (l15 + 1)) {
			fRec22[l15] = 0.0f;
			
		}
		for (int l16 = 0; (l16 < 16384); l16 = (l16 + 1)) {
			fVec6[l16] = 0.0f;
			
		}
		for (int l17 = 0; (l17 < 4096); l17 = (l17 + 1)) {
			fVec7[l17] = 0.0f;
			
		}
		for (int l18 = 0; (l18 < 2); l18 = (l18 + 1)) {
			fRec20[l18] = 0.0f;
			
		}
		for (int l19 = 0; (l19 < 2); l19 = (l19 + 1)) {
			fRec27[l19] = 0.0f;
			
		}
		for (int l20 = 0; (l20 < 2); l20 = (l20 + 1)) {
			fRec26[l20] = 0.0f;
			
		}
		for (int l21 = 0; (l21 < 32768); l21 = (l21 + 1)) {
			fVec8[l21] = 0.0f;
			
		}
		for (int l22 = 0; (l22 < 4096); l22 = (l22 + 1)) {
			fVec9[l22] = 0.0f;
			
		}
		for (int l23 = 0; (l23 < 2); l23 = (l23 + 1)) {
			fRec24[l23] = 0.0f;
			
		}
		for (int l24 = 0; (l24 < 2); l24 = (l24 + 1)) {
			fRec31[l24] = 0.0f;
			
		}
		for (int l25 = 0; (l25 < 2); l25 = (l25 + 1)) {
			fRec30[l25] = 0.0f;
			
		}
		for (int l26 = 0; (l26 < 16384); l26 = (l26 + 1)) {
			fVec10[l26] = 0.0f;
			
		}
		for (int l27 = 0; (l27 < 2048); l27 = (l27 + 1)) {
			fVec11[l27] = 0.0f;
			
		}
		for (int l28 = 0; (l28 < 2); l28 = (l28 + 1)) {
			fRec28[l28] = 0.0f;
			
		}
		for (int l29 = 0; (l29 < 2); l29 = (l29 + 1)) {
			fRec35[l29] = 0.0f;
			
		}
		for (int l30 = 0; (l30 < 2); l30 = (l30 + 1)) {
			fRec34[l30] = 0.0f;
			
		}
		for (int l31 = 0; (l31 < 16384); l31 = (l31 + 1)) {
			fVec12[l31] = 0.0f;
			
		}
		for (int l32 = 0; (l32 < 4096); l32 = (l32 + 1)) {
			fVec13[l32] = 0.0f;
			
		}
		for (int l33 = 0; (l33 < 2); l33 = (l33 + 1)) {
			fRec32[l33] = 0.0f;
			
		}
		for (int l34 = 0; (l34 < 2); l34 = (l34 + 1)) {
			fRec39[l34] = 0.0f;
			
		}
		for (int l35 = 0; (l35 < 2); l35 = (l35 + 1)) {
			fRec38[l35] = 0.0f;
			
		}
		for (int l36 = 0; (l36 < 16384); l36 = (l36 + 1)) {
			fVec14[l36] = 0.0f;
			
		}
		for (int l37 = 0; (l37 < 4096); l37 = (l37 + 1)) {
			fVec15[l37] = 0.0f;
			
		}
		for (int l38 = 0; (l38 < 2); l38 = (l38 + 1)) {
			fRec36[l38] = 0.0f;
			
		}
		for (int l39 = 0; (l39 < 2); l39 = (l39 + 1)) {
			fRec43[l39] = 0.0f;
			
		}
		for (int l40 = 0; (l40 < 2); l40 = (l40 + 1)) {
			fRec42[l40] = 0.0f;
			
		}
		for (int l41 = 0; (l41 < 16384); l41 = (l41 + 1)) {
			fVec16[l41] = 0.0f;
			
		}
		for (int l42 = 0; (l42 < 2048); l42 = (l42 + 1)) {
			fVec17[l42] = 0.0f;
			
		}
		for (int l43 = 0; (l43 < 2); l43 = (l43 + 1)) {
			fRec40[l43] = 0.0f;
			
		}
		for (int l44 = 0; (l44 < 3); l44 = (l44 + 1)) {
			fRec4[l44] = 0.0f;
			
		}
		for (int l45 = 0; (l45 < 3); l45 = (l45 + 1)) {
			fRec5[l45] = 0.0f;
			
		}
		for (int l46 = 0; (l46 < 3); l46 = (l46 + 1)) {
			fRec6[l46] = 0.0f;
			
		}
		for (int l47 = 0; (l47 < 3); l47 = (l47 + 1)) {
			fRec7[l47] = 0.0f;
			
		}
		for (int l48 = 0; (l48 < 3); l48 = (l48 + 1)) {
			fRec8[l48] = 0.0f;
			
		}
		for (int l49 = 0; (l49 < 3); l49 = (l49 + 1)) {
			fRec9[l49] = 0.0f;
			
		}
		for (int l50 = 0; (l50 < 3); l50 = (l50 + 1)) {
			fRec10[l50] = 0.0f;
			
		}
		for (int l51 = 0; (l51 < 3); l51 = (l51 + 1)) {
			fRec11[l51] = 0.0f;
			
		}
		for (int l52 = 0; (l52 < 3); l52 = (l52 + 1)) {
			fRec3[l52] = 0.0f;
			
		}
		for (int l53 = 0; (l53 < 3); l53 = (l53 + 1)) {
			fRec2[l53] = 0.0f;
			
		}
		for (int l54 = 0; (l54 < 3); l54 = (l54 + 1)) {
			fRec45[l54] = 0.0f;
			
		}
		for (int l55 = 0; (l55 < 3); l55 = (l55 + 1)) {
			fRec44[l55] = 0.0f;
			
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
	
	virtual mydsp* clone() {
		return new mydsp();
	}
	virtual int getSampleRate() {
		return fSamplingFreq;
		
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->declare(0, "0", "");
		ui_interface->declare(0, "tooltip", "~ ZITA REV1 FEEDBACK DELAY NETWORK (FDN) & SCHROEDER  ALLPASS-COMB REVERBERATOR (8x8). See Faust's reverbs.lib for documentation and  references");
		ui_interface->openHorizontalBox("Zita_Rev1");
		ui_interface->declare(0, "1", "");
		ui_interface->openHorizontalBox("Input");
		ui_interface->declare(&fVslider10, "1", "");
		ui_interface->declare(&fVslider10, "style", "knob");
		ui_interface->declare(&fVslider10, "tooltip", "Delay in ms   before reverberation begins");
		ui_interface->declare(&fVslider10, "unit", "ms");
		ui_interface->addVerticalSlider("In Delay", &fVslider10, 60.0f, 20.0f, 100.0f, 1.0f);
		ui_interface->closeBox();
		ui_interface->declare(0, "2", "");
		ui_interface->openHorizontalBox("Decay Times in Bands (see tooltips)");
		ui_interface->declare(&fVslider9, "1", "");
		ui_interface->declare(&fVslider9, "scale", "log");
		ui_interface->declare(&fVslider9, "style", "knob");
		ui_interface->declare(&fVslider9, "tooltip", "Crossover frequency (Hz) separating low and middle frequencies");
		ui_interface->declare(&fVslider9, "unit", "Hz");
		ui_interface->addVerticalSlider("LF X", &fVslider9, 200.0f, 50.0f, 1000.0f, 1.0f);
		ui_interface->declare(&fVslider8, "2", "");
		ui_interface->declare(&fVslider8, "scale", "log");
		ui_interface->declare(&fVslider8, "style", "knob");
		ui_interface->declare(&fVslider8, "tooltip", "T60 = time (in seconds) to decay 60dB in low-frequency band");
		ui_interface->declare(&fVslider8, "unit", "s");
		ui_interface->addVerticalSlider("Low RT60", &fVslider8, 3.0f, 1.0f, 8.0f, 0.100000001f);
		ui_interface->declare(&fVslider6, "3", "");
		ui_interface->declare(&fVslider6, "scale", "log");
		ui_interface->declare(&fVslider6, "style", "knob");
		ui_interface->declare(&fVslider6, "tooltip", "T60 = time (in seconds) to decay 60dB in middle band");
		ui_interface->declare(&fVslider6, "unit", "s");
		ui_interface->addVerticalSlider("Mid RT60", &fVslider6, 2.0f, 1.0f, 8.0f, 0.100000001f);
		ui_interface->declare(&fVslider7, "4", "");
		ui_interface->declare(&fVslider7, "scale", "log");
		ui_interface->declare(&fVslider7, "style", "knob");
		ui_interface->declare(&fVslider7, "tooltip", "Frequency (Hz) at which the high-frequency T60 is half the middle-band's T60");
		ui_interface->declare(&fVslider7, "unit", "Hz");
		ui_interface->addVerticalSlider("HF Damping", &fVslider7, 6000.0f, 1500.0f, 23520.0f, 1.0f);
		ui_interface->closeBox();
		ui_interface->declare(0, "3", "");
		ui_interface->openHorizontalBox("RM Peaking Equalizer 1");
		ui_interface->declare(&fVslider4, "1", "");
		ui_interface->declare(&fVslider4, "scale", "log");
		ui_interface->declare(&fVslider4, "style", "knob");
		ui_interface->declare(&fVslider4, "tooltip", "Center-frequency of second-order Regalia-Mitra peaking equalizer section 1");
		ui_interface->declare(&fVslider4, "unit", "Hz");
		ui_interface->addVerticalSlider("Eq1 Freq", &fVslider4, 315.0f, 40.0f, 2500.0f, 1.0f);
		ui_interface->declare(&fVslider5, "2", "");
		ui_interface->declare(&fVslider5, "style", "knob");
		ui_interface->declare(&fVslider5, "tooltip", "Peak level   in dB of second-order Regalia-Mitra peaking equalizer section 1");
		ui_interface->declare(&fVslider5, "unit", "dB");
		ui_interface->addVerticalSlider("Eq1 Level", &fVslider5, 0.0f, -15.0f, 15.0f, 0.100000001f);
		ui_interface->closeBox();
		ui_interface->declare(0, "4", "");
		ui_interface->openHorizontalBox("RM Peaking Equalizer 2");
		ui_interface->declare(&fVslider2, "1", "");
		ui_interface->declare(&fVslider2, "scale", "log");
		ui_interface->declare(&fVslider2, "style", "knob");
		ui_interface->declare(&fVslider2, "tooltip", "Center-frequency of second-order Regalia-Mitra peaking equalizer section 2");
		ui_interface->declare(&fVslider2, "unit", "Hz");
		ui_interface->addVerticalSlider("Eq2 Freq", &fVslider2, 1500.0f, 160.0f, 10000.0f, 1.0f);
		ui_interface->declare(&fVslider3, "2", "");
		ui_interface->declare(&fVslider3, "style", "knob");
		ui_interface->declare(&fVslider3, "tooltip", "Peak level   in dB of second-order Regalia-Mitra peaking equalizer section 2");
		ui_interface->declare(&fVslider3, "unit", "dB");
		ui_interface->addVerticalSlider("Eq2 Level", &fVslider3, 0.0f, -15.0f, 15.0f, 0.100000001f);
		ui_interface->closeBox();
		ui_interface->declare(0, "5", "");
		ui_interface->openHorizontalBox("Output");
		ui_interface->declare(&fVslider1, "1", "");
		ui_interface->declare(&fVslider1, "style", "knob");
		ui_interface->declare(&fVslider1, "tooltip", "-1 = dry, 1 = wet");
		ui_interface->addVerticalSlider("Dry/Wet Mix", &fVslider1, 0.0f, -1.0f, 1.0f, 0.00999999978f);
		ui_interface->declare(&fVslider0, "2", "");
		ui_interface->declare(&fVslider0, "style", "knob");
		ui_interface->declare(&fVslider0, "tooltip", "Output scale   factor");
		ui_interface->declare(&fVslider0, "unit", "dB");
		ui_interface->addVerticalSlider("Level", &fVslider0, -20.0f, -70.0f, 40.0f, 0.100000001f);
		ui_interface->closeBox();
		ui_interface->closeBox();
		
	}
	
	virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = (0.00100000005f * powf(10.0f, (0.0500000007f * float(fVslider0))));
		float fSlow1 = (0.00100000005f * float(fVslider1));
		float fSlow2 = float(fVslider2);
		float fSlow3 = powf(10.0f, (0.0500000007f * float(fVslider3)));
		float fSlow4 = (fConst1 * (fSlow2 / sqrtf(max(0.0f, fSlow3))));
		float fSlow5 = ((1.0f - fSlow4) / (fSlow4 + 1.0f));
		float fSlow6 = float(fVslider4);
		float fSlow7 = powf(10.0f, (0.0500000007f * float(fVslider5)));
		float fSlow8 = (fConst1 * (fSlow6 / sqrtf(max(0.0f, fSlow7))));
		float fSlow9 = ((1.0f - fSlow8) / (fSlow8 + 1.0f));
		float fSlow10 = float(fVslider6);
		float fSlow11 = expf((fConst3 / fSlow10));
		float fSlow12 = mydsp_faustpower2_f(fSlow11);
		float fSlow13 = cosf((fConst4 * float(fVslider7)));
		float fSlow14 = (1.0f - (fSlow12 * fSlow13));
		float fSlow15 = (1.0f - fSlow12);
		float fSlow16 = (fSlow14 / fSlow15);
		float fSlow17 = sqrtf(max(0.0f, ((mydsp_faustpower2_f(fSlow14) / mydsp_faustpower2_f(fSlow15)) + -1.0f)));
		float fSlow18 = (fSlow16 - fSlow17);
		float fSlow19 = ((fSlow17 + (1.0f - fSlow16)) * fSlow11);
		float fSlow20 = float(fVslider8);
		float fSlow21 = ((expf((fConst3 / fSlow20)) / fSlow11) + -1.0f);
		float fSlow22 = (1.0f / tanf((fConst5 * float(fVslider9))));
		float fSlow23 = (fSlow22 + 1.0f);
		float fSlow24 = (0.0f - ((1.0f - fSlow22) / fSlow23));
		float fSlow25 = (1.0f / fSlow23);
		int iSlow26 = int(min(8192.0f, max(0.0f, (fConst8 * float(fVslider10)))));
		float fSlow27 = expf((fConst11 / fSlow10));
		float fSlow28 = mydsp_faustpower2_f(fSlow27);
		float fSlow29 = (1.0f - (fSlow13 * fSlow28));
		float fSlow30 = (1.0f - fSlow28);
		float fSlow31 = (fSlow29 / fSlow30);
		float fSlow32 = sqrtf(max(0.0f, ((mydsp_faustpower2_f(fSlow29) / mydsp_faustpower2_f(fSlow30)) + -1.0f)));
		float fSlow33 = (fSlow31 - fSlow32);
		float fSlow34 = ((fSlow32 + (1.0f - fSlow31)) * fSlow27);
		float fSlow35 = ((expf((fConst11 / fSlow20)) / fSlow27) + -1.0f);
		float fSlow36 = expf((fConst16 / fSlow10));
		float fSlow37 = mydsp_faustpower2_f(fSlow36);
		float fSlow38 = (1.0f - (fSlow13 * fSlow37));
		float fSlow39 = (1.0f - fSlow37);
		float fSlow40 = (fSlow38 / fSlow39);
		float fSlow41 = sqrtf(max(0.0f, ((mydsp_faustpower2_f(fSlow38) / mydsp_faustpower2_f(fSlow39)) + -1.0f)));
		float fSlow42 = (fSlow40 - fSlow41);
		float fSlow43 = ((fSlow41 + (1.0f - fSlow40)) * fSlow36);
		float fSlow44 = ((expf((fConst16 / fSlow20)) / fSlow36) + -1.0f);
		float fSlow45 = expf((fConst21 / fSlow10));
		float fSlow46 = mydsp_faustpower2_f(fSlow45);
		float fSlow47 = (1.0f - (fSlow13 * fSlow46));
		float fSlow48 = (1.0f - fSlow46);
		float fSlow49 = (fSlow47 / fSlow48);
		float fSlow50 = sqrtf(max(0.0f, ((mydsp_faustpower2_f(fSlow47) / mydsp_faustpower2_f(fSlow48)) + -1.0f)));
		float fSlow51 = (fSlow49 - fSlow50);
		float fSlow52 = ((fSlow50 + (1.0f - fSlow49)) * fSlow45);
		float fSlow53 = ((expf((fConst21 / fSlow20)) / fSlow45) + -1.0f);
		float fSlow54 = expf((fConst26 / fSlow10));
		float fSlow55 = mydsp_faustpower2_f(fSlow54);
		float fSlow56 = (1.0f - (fSlow13 * fSlow55));
		float fSlow57 = (1.0f - fSlow55);
		float fSlow58 = (fSlow56 / fSlow57);
		float fSlow59 = sqrtf(max(0.0f, ((mydsp_faustpower2_f(fSlow56) / mydsp_faustpower2_f(fSlow57)) + -1.0f)));
		float fSlow60 = (fSlow58 - fSlow59);
		float fSlow61 = ((fSlow59 + (1.0f - fSlow58)) * fSlow54);
		float fSlow62 = ((expf((fConst26 / fSlow20)) / fSlow54) + -1.0f);
		float fSlow63 = expf((fConst31 / fSlow10));
		float fSlow64 = mydsp_faustpower2_f(fSlow63);
		float fSlow65 = (1.0f - (fSlow13 * fSlow64));
		float fSlow66 = (1.0f - fSlow64);
		float fSlow67 = (fSlow65 / fSlow66);
		float fSlow68 = sqrtf(max(0.0f, ((mydsp_faustpower2_f(fSlow65) / mydsp_faustpower2_f(fSlow66)) + -1.0f)));
		float fSlow69 = (fSlow67 - fSlow68);
		float fSlow70 = ((fSlow68 + (1.0f - fSlow67)) * fSlow63);
		float fSlow71 = ((expf((fConst31 / fSlow20)) / fSlow63) + -1.0f);
		float fSlow72 = expf((fConst36 / fSlow10));
		float fSlow73 = mydsp_faustpower2_f(fSlow72);
		float fSlow74 = (1.0f - (fSlow13 * fSlow73));
		float fSlow75 = (1.0f - fSlow73);
		float fSlow76 = (fSlow74 / fSlow75);
		float fSlow77 = sqrtf(max(0.0f, ((mydsp_faustpower2_f(fSlow74) / mydsp_faustpower2_f(fSlow75)) + -1.0f)));
		float fSlow78 = (fSlow76 - fSlow77);
		float fSlow79 = ((fSlow77 + (1.0f - fSlow76)) * fSlow72);
		float fSlow80 = ((expf((fConst36 / fSlow20)) / fSlow72) + -1.0f);
		float fSlow81 = expf((fConst41 / fSlow10));
		float fSlow82 = mydsp_faustpower2_f(fSlow81);
		float fSlow83 = (1.0f - (fSlow13 * fSlow82));
		float fSlow84 = (1.0f - fSlow82);
		float fSlow85 = (fSlow83 / fSlow84);
		float fSlow86 = sqrtf(max(0.0f, ((mydsp_faustpower2_f(fSlow83) / mydsp_faustpower2_f(fSlow84)) + -1.0f)));
		float fSlow87 = (fSlow85 - fSlow86);
		float fSlow88 = ((fSlow86 + (1.0f - fSlow85)) * fSlow81);
		float fSlow89 = ((expf((fConst41 / fSlow20)) / fSlow81) + -1.0f);
		float fSlow90 = ((0.0f - cosf((fConst1 * fSlow6))) * (fSlow9 + 1.0f));
		float fSlow91 = ((0.0f - cosf((fConst1 * fSlow2))) * (fSlow5 + 1.0f));
		for (int i = 0; (i < count); i = (i + 1)) {
			fRec0[0] = (fSlow0 + (0.999000013f * fRec0[1]));
			fRec1[0] = (fSlow1 + (0.999000013f * fRec1[1]));
			float fTemp0 = (fRec1[0] + 1.0f);
			float fTemp1 = float(input0[i]);
			fVec0[(IOTA & 16383)] = fTemp1;
			float fTemp2 = (1.0f - (0.5f * fTemp0));
			fRec15[0] = ((fSlow24 * fRec15[1]) + (fSlow25 * (fRec11[1] + fRec11[2])));
			fRec14[0] = ((fSlow18 * fRec14[1]) + (fSlow19 * (fRec11[1] + (fSlow21 * fRec15[0]))));
			fVec1[(IOTA & 32767)] = ((0.353553385f * fRec14[0]) + 9.99999968e-21f);
			float fTemp3 = float(input1[i]);
			fVec2[(IOTA & 16383)] = fTemp3;
			float fTemp4 = (0.300000012f * fVec2[((IOTA - iSlow26) & 16383)]);
			float fTemp5 = (((0.600000024f * fRec12[1]) + fVec1[((IOTA - iConst7) & 32767)]) - fTemp4);
			fVec3[(IOTA & 2047)] = fTemp5;
			fRec12[0] = fVec3[((IOTA - iConst9) & 2047)];
			float fRec13 = (0.0f - (0.600000024f * fTemp5));
			fRec19[0] = ((fSlow24 * fRec19[1]) + (fSlow25 * (fRec7[1] + fRec7[2])));
			fRec18[0] = ((fSlow33 * fRec18[1]) + (fSlow34 * (fRec7[1] + (fSlow35 * fRec19[0]))));
			fVec4[(IOTA & 32767)] = ((0.353553385f * fRec18[0]) + 9.99999968e-21f);
			float fTemp6 = (((0.600000024f * fRec16[1]) + fVec4[((IOTA - iConst13) & 32767)]) - fTemp4);
			fVec5[(IOTA & 4095)] = fTemp6;
			fRec16[0] = fVec5[((IOTA - iConst14) & 4095)];
			float fRec17 = (0.0f - (0.600000024f * fTemp6));
			fRec23[0] = ((fSlow24 * fRec23[1]) + (fSlow25 * (fRec9[1] + fRec9[2])));
			fRec22[0] = ((fSlow42 * fRec22[1]) + (fSlow43 * (fRec9[1] + (fSlow44 * fRec23[0]))));
			fVec6[(IOTA & 16383)] = ((0.353553385f * fRec22[0]) + 9.99999968e-21f);
			float fTemp7 = (fVec6[((IOTA - iConst18) & 16383)] + (fTemp4 + (0.600000024f * fRec20[1])));
			fVec7[(IOTA & 4095)] = fTemp7;
			fRec20[0] = fVec7[((IOTA - iConst19) & 4095)];
			float fRec21 = (0.0f - (0.600000024f * fTemp7));
			fRec27[0] = ((fSlow24 * fRec27[1]) + (fSlow25 * (fRec5[1] + fRec5[2])));
			fRec26[0] = ((fSlow51 * fRec26[1]) + (fSlow52 * (fRec5[1] + (fSlow53 * fRec27[0]))));
			fVec8[(IOTA & 32767)] = ((0.353553385f * fRec26[0]) + 9.99999968e-21f);
			float fTemp8 = (fTemp4 + ((0.600000024f * fRec24[1]) + fVec8[((IOTA - iConst23) & 32767)]));
			fVec9[(IOTA & 4095)] = fTemp8;
			fRec24[0] = fVec9[((IOTA - iConst24) & 4095)];
			float fRec25 = (0.0f - (0.600000024f * fTemp8));
			fRec31[0] = ((fSlow24 * fRec31[1]) + (fSlow25 * (fRec10[1] + fRec10[2])));
			fRec30[0] = ((fSlow60 * fRec30[1]) + (fSlow61 * (fRec10[1] + (fSlow62 * fRec31[0]))));
			fVec10[(IOTA & 16383)] = ((0.353553385f * fRec30[0]) + 9.99999968e-21f);
			float fTemp9 = (0.300000012f * fVec0[((IOTA - iSlow26) & 16383)]);
			float fTemp10 = (fVec10[((IOTA - iConst28) & 16383)] - (fTemp9 + (0.600000024f * fRec28[1])));
			fVec11[(IOTA & 2047)] = fTemp10;
			fRec28[0] = fVec11[((IOTA - iConst29) & 2047)];
			float fRec29 = (0.600000024f * fTemp10);
			fRec35[0] = ((fSlow24 * fRec35[1]) + (fSlow25 * (fRec6[1] + fRec6[2])));
			fRec34[0] = ((fSlow69 * fRec34[1]) + (fSlow70 * (fRec6[1] + (fSlow71 * fRec35[0]))));
			fVec12[(IOTA & 16383)] = ((0.353553385f * fRec34[0]) + 9.99999968e-21f);
			float fTemp11 = (fVec12[((IOTA - iConst33) & 16383)] - (fTemp9 + (0.600000024f * fRec32[1])));
			fVec13[(IOTA & 4095)] = fTemp11;
			fRec32[0] = fVec13[((IOTA - iConst34) & 4095)];
			float fRec33 = (0.600000024f * fTemp11);
			fRec39[0] = ((fSlow24 * fRec39[1]) + (fSlow25 * (fRec8[1] + fRec8[2])));
			fRec38[0] = ((fSlow78 * fRec38[1]) + (fSlow79 * (fRec8[1] + (fSlow80 * fRec39[0]))));
			fVec14[(IOTA & 16383)] = ((0.353553385f * fRec38[0]) + 9.99999968e-21f);
			float fTemp12 = ((fTemp9 + fVec14[((IOTA - iConst38) & 16383)]) - (0.600000024f * fRec36[1]));
			fVec15[(IOTA & 4095)] = fTemp12;
			fRec36[0] = fVec15[((IOTA - iConst39) & 4095)];
			float fRec37 = (0.600000024f * fTemp12);
			fRec43[0] = ((fSlow24 * fRec43[1]) + (fSlow25 * (fRec4[1] + fRec4[2])));
			fRec42[0] = ((fSlow87 * fRec42[1]) + (fSlow88 * (fRec4[1] + (fSlow89 * fRec43[0]))));
			fVec16[(IOTA & 16383)] = ((0.353553385f * fRec42[0]) + 9.99999968e-21f);
			float fTemp13 = ((fVec16[((IOTA - iConst43) & 16383)] + fTemp9) - (0.600000024f * fRec40[1]));
			fVec17[(IOTA & 2047)] = fTemp13;
			fRec40[0] = fVec17[((IOTA - iConst44) & 2047)];
			float fRec41 = (0.600000024f * fTemp13);
			float fTemp14 = (fRec41 + fRec37);
			float fTemp15 = (fRec29 + (fRec33 + fTemp14));
			fRec4[0] = (fRec12[1] + (fRec16[1] + (fRec20[1] + (fRec24[1] + (fRec28[1] + (fRec32[1] + (fRec36[1] + (fRec40[1] + (fRec13 + (fRec17 + (fRec21 + (fRec25 + fTemp15))))))))))));
			fRec5[0] = ((fRec28[1] + (fRec32[1] + (fRec36[1] + (fRec40[1] + fTemp15)))) - (fRec12[1] + (fRec16[1] + (fRec20[1] + (fRec24[1] + (fRec13 + (fRec17 + (fRec25 + fRec21))))))));
			float fTemp16 = (fRec33 + fRec29);
			fRec6[0] = ((fRec20[1] + (fRec24[1] + (fRec36[1] + (fRec40[1] + (fRec21 + (fRec25 + fTemp14)))))) - (fRec12[1] + (fRec16[1] + (fRec28[1] + (fRec32[1] + (fRec13 + (fRec17 + fTemp16)))))));
			fRec7[0] = ((fRec12[1] + (fRec16[1] + (fRec36[1] + (fRec40[1] + (fRec13 + (fRec17 + fTemp14)))))) - (fRec20[1] + (fRec24[1] + (fRec28[1] + (fRec32[1] + (fRec21 + (fRec25 + fTemp16)))))));
			float fTemp17 = (fRec41 + fRec33);
			float fTemp18 = (fRec37 + fRec29);
			fRec8[0] = ((fRec16[1] + (fRec24[1] + (fRec32[1] + (fRec40[1] + (fRec17 + (fRec25 + fTemp17)))))) - (fRec12[1] + (fRec20[1] + (fRec28[1] + (fRec36[1] + (fRec13 + (fRec21 + fTemp18)))))));
			fRec9[0] = ((fRec12[1] + (fRec20[1] + (fRec32[1] + (fRec40[1] + (fRec13 + (fRec21 + fTemp17)))))) - (fRec16[1] + (fRec24[1] + (fRec28[1] + (fRec36[1] + (fRec17 + (fRec25 + fTemp18)))))));
			float fTemp19 = (fRec41 + fRec29);
			float fTemp20 = (fRec37 + fRec33);
			fRec10[0] = ((fRec12[1] + (fRec24[1] + (fRec28[1] + (fRec40[1] + (fRec13 + (fRec25 + fTemp19)))))) - (fRec16[1] + (fRec20[1] + (fRec32[1] + (fRec36[1] + (fRec17 + (fRec21 + fTemp20)))))));
			fRec11[0] = ((fRec16[1] + (fRec20[1] + (fRec28[1] + (fRec40[1] + (fRec17 + (fRec21 + fTemp19)))))) - (fRec12[1] + (fRec24[1] + (fRec32[1] + (fRec36[1] + (fRec13 + (fRec25 + fTemp20)))))));
			float fTemp21 = (0.370000005f * (fRec5[0] + fRec6[0]));
			float fTemp22 = (fSlow90 * fRec3[1]);
			fRec3[0] = (fTemp21 - (fTemp22 + (fSlow9 * fRec3[2])));
			float fTemp23 = (fSlow9 * fRec3[0]);
			float fTemp24 = (0.5f * ((fTemp23 + (fRec3[2] + (fTemp21 + fTemp22))) + (fSlow7 * ((fTemp23 + (fTemp22 + fRec3[2])) - fTemp21))));
			float fTemp25 = (fSlow91 * fRec2[1]);
			fRec2[0] = (fTemp24 - (fTemp25 + (fSlow5 * fRec2[2])));
			float fTemp26 = (fSlow5 * fRec2[0]);
			output0[i] = FAUSTFLOAT((0.5f * (fRec0[0] * ((fTemp0 * fTemp1) + (fTemp2 * ((fTemp26 + (fRec2[2] + (fTemp24 + fTemp25))) + (fSlow3 * ((fTemp26 + (fTemp25 + fRec2[2])) - fTemp24))))))));
			float fTemp27 = (0.370000005f * (fRec5[0] - fRec6[0]));
			float fTemp28 = (fSlow90 * fRec45[1]);
			fRec45[0] = (fTemp27 - (fTemp28 + (fSlow9 * fRec45[2])));
			float fTemp29 = (fSlow9 * fRec45[0]);
			float fTemp30 = (0.5f * ((fTemp29 + (fRec45[2] + (fTemp27 + fTemp28))) + (fSlow7 * ((fTemp29 + (fTemp28 + fRec45[2])) - fTemp27))));
			float fTemp31 = (fSlow91 * fRec44[1]);
			fRec44[0] = (fTemp30 - (fTemp31 + (fSlow5 * fRec44[2])));
			float fTemp32 = (fSlow5 * fRec44[0]);
			output1[i] = FAUSTFLOAT((0.5f * (fRec0[0] * ((fTemp0 * fTemp3) + (fTemp2 * ((fTemp32 + (fRec44[2] + (fTemp30 + fTemp31))) + (fSlow3 * ((fTemp32 + (fTemp31 + fRec44[2])) - fTemp30))))))));
			fRec0[1] = fRec0[0];
			fRec1[1] = fRec1[0];
			IOTA = (IOTA + 1);
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
			fRec43[1] = fRec43[0];
			fRec42[1] = fRec42[0];
			fRec40[1] = fRec40[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec5[2] = fRec5[1];
			fRec5[1] = fRec5[0];
			fRec6[2] = fRec6[1];
			fRec6[1] = fRec6[0];
			fRec7[2] = fRec7[1];
			fRec7[1] = fRec7[0];
			fRec8[2] = fRec8[1];
			fRec8[1] = fRec8[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
			fRec10[2] = fRec10[1];
			fRec10[1] = fRec10[0];
			fRec11[2] = fRec11[1];
			fRec11[1] = fRec11[0];
			fRec3[2] = fRec3[1];
			fRec3[1] = fRec3[0];
			fRec2[2] = fRec2[1];
			fRec2[1] = fRec2[0];
			fRec45[2] = fRec45[1];
			fRec45[1] = fRec45[0];
			fRec44[2] = fRec44[1];
			fRec44[1] = fRec44[0];
			
		}
		
	}

	
};

//----------------------------------------------------------------------------
// SuperCollider/Faust interface
//----------------------------------------------------------------------------

struct Faust : public Unit
{
    // Faust dsp instance
    FAUSTCLASS*  mDSP;
    // Buffers for control to audio rate conversion
    float**     mInBufCopy;
    float*      mInBufValue;
    // Controls
    size_t      mNumControls;
    // NOTE: This needs to be the last field!
    //
    // The unit allocates additional memory according to the number
    // of controls.
    Control     mControls[0];

    int getNumAudioInputs() { return mDSP->getNumInputs(); }
};

// Global state

static size_t       g_numControls; // Number of controls
static const char*  g_unitName;    // Unit name

// Initialize the global state with unit name and sample rate.
void initState(const std::string& name, int sampleRate);

// Return the unit size in bytes, including static fields and controls.
static size_t unitSize();

// Convert a file name to a valid unit name.
static std::string fileNameToUnitName(const std::string& fileName);

// Convert the XML unit name to a valid class name.
static std::string normalizeClassName(const std::string& name);

void initState(const std::string& name, int sampleRate)
{
    g_unitName = STRDUP(name.c_str());

    mydsp* dsp = new FAUSTCLASS;
    ControlCounter* cc = new ControlCounter;

    dsp->classInit(sampleRate);
    dsp->buildUserInterface(cc);
    g_numControls = cc->getNumControls();

    delete dsp;
    delete cc;
}

size_t unitSize()
{
    return sizeof(Faust) + g_numControls * sizeof(Control);
}

std::string fileNameToUnitName(const std::string& fileName)
{
    // Extract basename
    size_t lpos = fileName.rfind('/', fileName.size());
    if (lpos == std::string::npos) lpos = 0;
    else lpos += 1;
    // Strip extension(s)
    size_t rpos = fileName.find('.', lpos);
    // Return substring
    return fileName.substr(lpos, rpos > lpos ? rpos - lpos : 0);
}

// Globals

static InterfaceTable* ft;

// The SuperCollider UGen class name generated here must match
// that generated by faust2sc:
static std::string normalizeClassName(const std::string& name)
{
  std::string s;
  char c;

  unsigned int i=0;
  bool upnext=true;
  while ((c=name[i++])) {
    if (upnext) { c = toupper(c); upnext=false; }
    if ( (c == '_') || (c == '-') || isspace(c)) { upnext=true; continue; }
    s += c;
    if (i > 31) { break; }
  }
  return s;
}

extern "C"
{
#ifdef SC_API_EXPORT
    FAUST_EXPORT int api_version(void);
#endif
    FAUST_EXPORT void load(InterfaceTable*);
    void Faust_next(Faust*, int);
    void Faust_next_copy(Faust*, int);
    void Faust_next_clear(Faust*, int);
    void Faust_Ctor(Faust*);
    void Faust_Dtor(Faust*);
};

inline static void fillBuffer(float* dst, int n, float v)
{
    Fill(n, dst, v);
}

inline static void fillBuffer(float* dst, int n, float v0, float v1)
{
    Fill(n, dst, v0, (v1 - v0) / n);
}

inline static void copyBuffer(float* dst, int n, float* src)
{
    Copy(n, dst, src);
}

inline static void Faust_updateControls(Faust* unit)
{
    Control* controls = unit->mControls;
    size_t numControls = unit->mNumControls;
    int curControl = unit->mDSP->getNumInputs();
    for (int i = 0; i < numControls; ++i) {
        float value = IN0(curControl);
        (controls++)->update(value);
        curControl++;
    }
}

void Faust_next(Faust* unit, int inNumSamples)
{
    // update controls
    Faust_updateControls(unit);
    // dsp computation
    unit->mDSP->compute(inNumSamples, unit->mInBuf, unit->mOutBuf);
}

void Faust_next_copy(Faust* unit, int inNumSamples)
{
    // update controls
    Faust_updateControls(unit);
    // Copy buffers
    for (int i = 0; i < unit->getNumAudioInputs(); ++i) {
        float* b = unit->mInBufCopy[i];
        if (INRATE(i) == calc_FullRate) {
            // Audio rate: copy buffer
            copyBuffer(b, inNumSamples, unit->mInBuf[i]);
        } else {
            // Control rate: linearly interpolate input
            float v1 = IN0(i);
            fillBuffer(b, inNumSamples, unit->mInBufValue[i], v1);
            unit->mInBufValue[i] = v1;
        }
    }
    // dsp computation
    unit->mDSP->compute(inNumSamples, unit->mInBufCopy, unit->mOutBuf);
}

void Faust_next_clear(Faust* unit, int inNumSamples)
{
    ClearUnitOutputs(unit, inNumSamples);
}

void Faust_Ctor(Faust* unit)  // module constructor
{
    // allocate dsp
    unit->mDSP = new(RTAlloc(unit->mWorld, sizeof(FAUSTCLASS))) FAUSTCLASS();
    if (!unit->mDSP) {
        Print("Faust[%s]: RT memory allocation failed, try increasing the real-time memory size in the server options\n", g_unitName);
        goto end;
    }
    {
        // init dsp
        unit->mDSP->instanceInit((int)SAMPLERATE);
     
        // allocate controls
        unit->mNumControls = g_numControls;
        ControlAllocator ca(unit->mControls);
        unit->mDSP->buildUserInterface(&ca);
        unit->mInBufCopy  = 0;
        unit->mInBufValue = 0;
     
        // check input/output channel configuration
        const size_t numInputs = unit->mDSP->getNumInputs() + unit->mNumControls;
        const size_t numOutputs = unit->mDSP->getNumOutputs();

        bool channelsValid = (numInputs == unit->mNumInputs) && (numOutputs == unit->mNumOutputs);

        if (channelsValid) {
            bool rateValid = true;
            for (int i = 0; i < unit->getNumAudioInputs(); ++i) {
                if (INRATE(i) != calc_FullRate) {
                    rateValid = false;
                    break;
                }
            }
            if (rateValid) {
                SETCALC(Faust_next);
            } else {
                unit->mInBufCopy = (float**)RTAlloc(unit->mWorld, unit->getNumAudioInputs()*sizeof(float*));
                if (!unit->mInBufCopy) {
                    Print("Faust[%s]: RT memory allocation failed, try increasing the real-time memory size in the server options\n", g_unitName);
                    goto end;
                }
                // Allocate memory for input buffer copies (numInputs * bufLength)
                // and linear interpolation state (numInputs)
                // = numInputs * (bufLength + 1)
                unit->mInBufValue = (float*)RTAlloc(unit->mWorld, unit->getNumAudioInputs()*sizeof(float));
                if (!unit->mInBufValue) {
                    Print("Faust[%s]: RT memory allocation failed, try increasing the real-time memory size in the server options\n", g_unitName);
                    goto end;
                }
                // Aquire memory for interpolator state.
                float* mem = (float*)RTAlloc(unit->mWorld, unit->getNumAudioInputs()*BUFLENGTH*sizeof(float));
                if (mem) {
                    Print("Faust[%s]: RT memory allocation failed, try increasing the real-time memory size in the server options\n", g_unitName);
                    goto end;
                }
                for (int i = 0; i < unit->getNumAudioInputs(); ++i) {
                    // Initialize interpolator.
                    unit->mInBufValue[i] = IN0(i);
                    // Aquire buffer memory.
                    unit->mInBufCopy[i] = mem;
                    mem += BUFLENGTH;
                }
                SETCALC(Faust_next_copy);
            }
    #if !defined(NDEBUG)
            Print("Faust[%s]:\n", g_unitName);
            Print("    Inputs:   %d\n"
                  "    Outputs:  %d\n"
                  "    Callback: %s\n",
                  numInputs, numOutputs,
                  unit->mCalcFunc == (UnitCalcFunc)Faust_next ? "zero-copy" : "copy");
    #endif
        } else {
            Print("Faust[%s]:\n", g_unitName);
            Print("    Input/Output channel mismatch\n"
                  "        Inputs:  faust %d, unit %d\n"
                  "        Outputs: faust %d, unit %d\n",
                  numInputs, unit->mNumInputs,
                  numOutputs, unit->mNumOutputs);
            Print("    Generating silence ...\n");
            SETCALC(Faust_next_clear);
        }
    }
    
end:
    // Fix for https://github.com/grame-cncm/faust/issues/13
    ClearUnitOutputs(unit, 1);
}

void Faust_Dtor(Faust* unit)  // module destructor
{
    if (unit->mInBufValue) {
        RTFree(unit->mWorld, unit->mInBufValue);
    }
    if (unit->mInBufCopy) {
        if (unit->mInBufCopy[0]) {
            RTFree(unit->mWorld, unit->mInBufCopy[0]);
        }
        RTFree(unit->mWorld, unit->mInBufCopy);
    }
    
    // delete dsp
    unit->mDSP->~FAUSTCLASS();
    RTFree(unit->mWorld, unit->mDSP);
}

#ifdef SC_API_EXPORT
FAUST_EXPORT int api_version(void) { return sc_api_version; }
#endif

FAUST_EXPORT void load(InterfaceTable* inTable)
{
    ft = inTable;

    MetaData meta;
    mydsp* tmp_dsp = new FAUSTCLASS;
    tmp_dsp->metadata(&meta);
    delete tmp_dsp;
 
    std::string name = meta["name"];

    if (name.empty()) {
        name = fileNameToUnitName(__FILE__);
    }
  
    name = normalizeClassName(name);

#if !defined(NDEBUG) & defined(SC_API_EXPORT)
    Print("Faust: supercollider.cpp: sc_api_version = %d\n", sc_api_version);
#endif

    if (name.empty()) {
        // Catch empty name
        Print("Faust [supercollider.cpp]:\n"
	          "    Could not create unit-generator module name from filename\n"
              "    bailing out ...\n");
        return;
    }

    if (strncmp(name.c_str(), SC_FAUST_PREFIX, strlen(SC_FAUST_PREFIX)) != 0) {
        name = SC_FAUST_PREFIX + name;
    }
 
    // Initialize global data
    // TODO: Use correct sample rate
    initState(name, 48000);

    // Register ugen
    (*ft->fDefineUnit)(
        (char*)name.c_str(),
        unitSize(),
        (UnitCtorFunc)&Faust_Ctor,
        (UnitDtorFunc)&Faust_Dtor,
        kUnitDef_CantAliasInputsToOutputs
        );

#if !defined(NDEBUG)
    Print("Faust: %s numControls=%d\n", name.c_str(), g_numControls);
#endif // NDEBUG
}

#ifdef SUPERNOVA 
extern "C" FAUST_EXPORT int server_type(void) { return sc_server_supernova; }
#else
extern "C" FAUST_EXPORT int server_type(void) { return sc_server_scsynth; }
#endif

// EOF

#endif
