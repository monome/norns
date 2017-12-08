#include "SC_PlugIn.h"

#include "CutFadeLoopLogic.h"

#define CHECK_BUFFER_DATA \
if (!bufData) { \
	if(unit->mWorld->mVerbosity > -1 && !unit->mDone && (unit->m_failedBufNum != fbufnum)) { \
		Print("Buffer UGen: no buffer data\n"); \
		unit->m_failedBufNum = fbufnum; \
	} \
	ClearUnitOutputs(unit, inNumSamples); \
	return; \
} else { \
	if (bufChannels != numOutputs) { \
		if(unit->mWorld->mVerbosity > -1 && !unit->mDone && (unit->m_failedBufNum != fbufnum)) { \
			Print("Buffer UGen channel mismatch: expected %i, yet buffer has %i channels\n", \
				  numOutputs, bufChannels); \
			unit->m_failedBufNum = fbufnum; \
			} \
		} \
} \

static InterfaceTable *ft;

struct CutFadeLoop : public Unit {
   float m_fbufnum;
    float m_failedBufNum;
    SndBuf *m_buf;
    float prevTrig;
    CutFadeLoopLogic cutfade;
};


static void CutFadeLoop_next(CutFadeLoop *unit, int inNumSamples);
static void CutFadeLoop_Ctor(CutFadeLoop* unit);
//static void CutFadeLoop_Dtor(CutFadeLoop* unit);


void CutFadeLoop_Ctor(CutFadeLoop* unit) {
    Print("CutFadeLoop_Ctor() : samplerate %f \n", SAMPLERATE);
    unit->cutfade.setSampleRate(SAMPLERATE);
    // unit->cutfade.setRate(1.f);
    // unit->cutfade.setFadeTime(0.1);
    // unit->cutfade.setLoopStartSeconds(4.f);
    // unit->cutfade.setLoopEndSeconds(8.f);
    // unit->cutfade.setLoopFlag(true);
    // unit->cutfade.resetPos();

    unit->m_fbufnum = -1e9f;
    unit->m_failedBufNum = -1e9f;
    SETCALC(CutFadeLoop_next);
    CutFadeLoop_next(unit, 1);
}

//// this must be named PluginName_Dtor.
//void CutFadeLoop_Dtor(CutFadeLoop* unit) {
//}

void CutFadeLoop_next(CutFadeLoop *unit, int inNumSamples)
{
    GET_BUF_SHARED
    uint32 numOutputs = unit->mNumOutputs;
    CHECK_BUFFER_DATA

    /*
    if(!bufData) {
        Print("no buffer data; bufnum: %f\n", fbufnum);
        ClearUnitOutputs(unit, inNumSamples);
        return;
    }
*/
    float *out = OUT(0);
    // float *phase = OUT(1);

//    Print("buf pointer: %08x\n", bufData);

    unit->cutfade.setBuffer(bufData, bufFrames);

#if 1
    float trig = IN0(1);
    float rate = IN0(2);
    float start = IN0(3);
    float end = IN0(4);
    float fade = IN0(5);
    float loop = IN0(6);

    unit->cutfade.setSampleRate(SAMPLERATE); // ???
    unit->cutfade.setRate(rate);
    unit->cutfade.setLoopStartSeconds(start);
    unit->cutfade.setLoopEndSeconds(end);
    unit->cutfade.setFadeTime(fade);
    unit->cutfade.setLoopFlag(loop > 0);

    if((trig > 0.f) && (unit->prevTrig <= 0.f)) {
        unit->cutfade.resetPos();
    }
    unit->prevTrig = trig;
#endif

    for (int i = 0; i < inNumSamples; ++i) {
#if 0
        unit->cutfade.nextSample( &(out[i]), phase+i);
#endif

#if 1
        unit->cutfade.nextSample( &(out[i]), nullptr);
#endif
    }
}

PluginLoad(CutFadeLoop)
{
    ft = inTable;
    DefineSimpleUnit(CutFadeLoop);
    Print("PluginLoad(CutFadeLoop)\n");
}