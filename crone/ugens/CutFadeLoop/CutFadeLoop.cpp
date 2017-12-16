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
	if (bufChannels != (numOutputs-2)) { \
		if(unit->mWorld->mVerbosity > -1 && !unit->mDone && (unit->m_failedBufNum != fbufnum)) { \
			Print("Buffer UGen channel mismatch: expected %i, yet buffer has %i channels\n", \
				  numOutputs, bufChannels); \
			unit->m_failedBufNum = fbufnum; \
        } \
    } \
} \

static InterfaceTable *ft;

struct CutFadeLoop : public Unit {
    float prevTrig;
   float m_fbufnum;
    float m_failedBufNum;
    SndBuf *m_buf;
    CutFadeLoopLogic cutfade; // NB: constructor is never called on this field!
};


static void CutFadeLoop_next(CutFadeLoop *unit, int inNumSamples);
static void CutFadeLoop_Ctor(CutFadeLoop* unit);

void CutFadeLoop_Ctor(CutFadeLoop* unit) {
    Print("CutFadeLoop_Ctor() : samplerate %f \n", SAMPLERATE);
    unit->cutfade.init();
    unit->cutfade.setSampleRate(SAMPLERATE);
    unit->m_fbufnum = -1e9f;
    unit->m_failedBufNum = -1e9f;
    unit->prevTrig = 0.f;
    SETCALC(CutFadeLoop_next);
    CutFadeLoop_next(unit, 1);
}

void CutFadeLoop_next(CutFadeLoop *unit, int inNumSamples)
{
    GET_BUF_SHARED
    uint32 numOutputs = unit->mNumOutputs;
    //Print("numOutputs: %d\n", numOutputs);
    CHECK_BUFFER_DATA
    unit->cutfade.setBuffer(bufData, bufFrames);


    float *phase_out = OUT(0);
    float *trig_out = OUT(1);
    float *snd_out = OUT(2);

    float trig = IN0(1);
    float rate = IN0(2);
    float start = IN0(3);
    float end = IN0(4);
    float fade = IN0(5);
    float loop = IN0(6);

    // Print("Rate: %f\n", rate);

    unit->cutfade.setRate(rate);
    unit->cutfade.setLoopStartSeconds(start);
    unit->cutfade.setLoopEndSeconds(end);
    unit->cutfade.setFadeTime(fade);
    unit->cutfade.setLoopFlag(loop > 0);

    if((trig > 0.f) && (unit->prevTrig <= 0.f)) {
         unit->cutfade.cutToStart();
    }
    unit->prevTrig = trig;

    float snd, phi, tr;
    for (int i=0; i<inNumSamples; ++i) {
      unit->cutfade.nextSample( &phi, &tr, &snd);
        phase_out[i] = phi;
        trig_out[i] = tr;
        snd_out[i] = snd;
    }
}

PluginLoad(CutFadeLoop)
{
    ft = inTable;
    DefineSimpleUnit(CutFadeLoop);
    Print("PluginLoad(CutFadeLoop)\n");
}
