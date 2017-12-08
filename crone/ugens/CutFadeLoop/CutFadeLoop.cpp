#include "SC_PlugIn.h"

#include "CutFadeLoopLogic.h"

#define CHECK_BUF \
	if (!bufData) { \
		unit->mDone = true; \
		ClearUnitOutputs(unit, inNumSamples); \
		return; \
	}

static InterfaceTable *ft;

struct CutFadeLoop : public Unit {

    float m_fbufnum;
    SndBuf *m_buf;
    bool prevTrig;
    CutFadeLoopLogic cutfade;
};


static void CutFadeLoop_next(CutFadeLoop *unit, int inNumSamples);
static void CutFadeLoop_Ctor(CutFadeLoop* unit);
static void CutFadeLoop_Dtor(CutFadeLoop* unit);


void CutFadeLoop_Ctor(CutFadeLoop* unit) {
    SETCALC(CutFadeLoop_next);
    CutFadeLoop_next(unit, 1);

}

// this must be named PluginName_Dtor.
void CutFadeLoop_Dtor(CutFadeLoop* unit) {

    // Free the memory.
    // RTFree(unit->mWorld, unit->buf);
}

void CutFadeLoop_next(CutFadeLoop *unit, int inNumSamples)
{
    float *out = OUT(0);
    float *phase = OUT(1);

    float rate = IN0(1);
    bool trig = IN0(2) > 0.f;
    float start = IN0(3);
    float end = IN0(4);
    float fade = IN0(5);
    float loop = IN0(6);


    unit->cutfade.setRate(rate);
    unit->cutfade.setLoopStartSeconds(start);
    unit->cutfade.setLoopEndSeconds(end);
    unit->cutfade.setFadeTime(fade);
    unit->cutfade.setLoopFlag(loop > 0);

    if(trig && !(unit->prevTrig)) {
        unit->cutfade.resetPos();
    }
    unit->prevTrig = trig;

    GET_BUF
    CHECK_BUF

    unit->cutfade.setBuffer(bufData, bufFrames);

    for (int i = 0; i < inNumSamples; i++) {
        unit->cutfade.nextSample(out, phase);
    }
}

PluginLoad(CutFadeLoop)
{
    ft = inTable;
    // ATTENTION! This has changed!
    // In the previous examples this was DefineSimpleUnit.
    DefineDtorUnit(CutFadeLoop);
}