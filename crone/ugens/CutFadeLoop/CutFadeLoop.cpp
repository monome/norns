#include "SC_PlugIn.h"

#include "CutFadeLoopLogic.h"

#define CHECK_BUF \
	if (!bufData) { \
		unit->mDone = true; \
		ClearUnitOutputs(unit, inNumSamples); \
		return; \
	}

//
//#define SETUP_OUT(offset) \
//	if (unit->mNumOutputs != bufChannels) { \
//		ClearUnitOutputs(unit, inNumSamples); \
//		return; \
//	} \
//	float *const * const out = &OUT(offset);

static InterfaceTable *ft;

struct CutFadeLoop : public Unit {
   float m_fbufnum;
    SndBuf *m_buf;
    float prevTrig;
    CutFadeLoopLogic cutfade;
};


static void CutFadeLoop_next(CutFadeLoop *unit, int inNumSamples);
static void CutFadeLoop_Ctor(CutFadeLoop* unit);
static void CutFadeLoop_Dtor(CutFadeLoop* unit);


void CutFadeLoop_Ctor(CutFadeLoop* unit) {
    SETCALC(CutFadeLoop_next);
    unit->cutfade.setSampleRate(SAMPLERATE);
    CutFadeLoop_next(unit, 1);
}

// this must be named PluginName_Dtor.
void CutFadeLoop_Dtor(CutFadeLoop* unit) {

    // Free the memory.
    // RTFree(unit->mWorld, unit->buf);
}

void CutFadeLoop_next(CutFadeLoop *unit, int inNumSamples)
{
    GET_BUF
    CHECK_BUF
    unit->cutfade.setBuffer(bufData, bufFrames);
    //SETUP_OUT(0)

    float trig = IN0(1);
    float rate = IN0(2);
    float start = IN0(3);
    float end = IN0(4);
    float fade = IN0(5);
    float loop = IN0(6);

    float *out = OUT(0);
//    float *phase = OUT(1);

//    Print("rate: %f; trig: %f; start: %f; end: %f; fade: %f; loop:%f\n",
//          rate, trig?1:0, start, end, fade, loop
//    );

    unit->cutfade.setRate(rate);
    unit->cutfade.setLoopStartSeconds(start);
    unit->cutfade.setLoopEndSeconds(end);
    unit->cutfade.setFadeTime(fade);
    unit->cutfade.setLoopFlag(loop > 0);

    if((trig > 0.f) && (unit->prevTrig) <= 0.f) {
        unit->cutfade.resetPos();
    }
    unit->prevTrig = trig;

//    Print("buf pointer: %08x\n", bufData);

    for (int i = 0; i < inNumSamples; i++) {
        //unit->cutfade.nextSample(out+i, phase+i);
        unit->cutfade.nextSample(out+i, nullptr);
    }
}

PluginLoad(CutFadeLoop)
{
    ft = inTable;
    // ATTENTION! This has changed!
    // In the previous examples this was DefineSimpleUnit.
    DefineDtorUnit(CutFadeLoop);
    Print("PluginLoad(CutFadeLoop)\n");
}