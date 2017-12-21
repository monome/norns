#include "SC_PlugIn.h"

#include "CutFadeVoiceLogic.h"

struct CutFadeVoice : public Unit {
    float prevTrig;
    float m_fbufnum;
    float m_failedBufNum;
    SndBuf *m_buf;
    CutFadeVoiceLogic cutfade; // NB: constructor is never called on this field!
};

static InterfaceTable *ft;

static inline bool checkBuffer(Unit *unit, const float *bufData, uint32 bufChannels,
                               uint32 expectedChannels, int inNumSamples) {
    if (!bufData)
        goto handle_failure;

    if (expectedChannels > bufChannels) {
        if (unit->mWorld->mVerbosity > -1 && !unit->mDone)
            Print("Buffer UGen channel mismatch: expected %i, yet buffer has %i channels\n",
                  expectedChannels, bufChannels);
        goto handle_failure;
    }
    return true;
    handle_failure:
    unit->mDone = true;
    ClearUnitOutputs(unit, inNumSamples);
    return false;
}

static void CutFadeVoice_next(CutFadeVoice *unit, int inNumSamples);

static void CutFadeVoice_Ctor(CutFadeVoice *unit);

void CutFadeVoice_Ctor(CutFadeVoice *unit) {
    Print("CutFadeVoice_Ctor() : samplerate %f \n", SAMPLERATE);
    unit->cutfade.init();
    unit->cutfade.setSampleRate(SAMPLERATE);
    unit->m_fbufnum = -1e9f;
    unit->m_failedBufNum = -1e9f;
    unit->prevTrig = 0.f;
    SETCALC(CutFadeVoice_next);
    CutFadeVoice_next(unit, 1);
}

void CutFadeVoice_next(CutFadeVoice *unit, int inNumSamples) {
    GET_BUF;
    uint32 numOutputs = unit->mNumOutputs;
    uint32 numInputChannels = unit->mNumInputs - 12;

    if (!checkBuffer(unit, bufData, bufChannels, numInputChannels, inNumSamples))
        return;

    unit->cutfade.setBuffer(bufData, bufFrames);

    float *phase_out = OUT(0);
    float *trig_out = OUT(1);
    float *snd_out = OUT(2);

    const float *in = IN(1);

    // Print("CutFadeVoice input: %f\n", in);

    float trig = IN0(2);
    float* rate = IN(3);
    float start = IN0(4);
    float end = IN0(5);
    float fade = IN0(6);
    float loop = IN0(7);


    const float *rec = IN(8);
    const float *pre = IN(9);

    float fadeRec = IN0(10);
    float fadePre = IN0(11);

    float recRun = IN0(12);

    unit->cutfade.setLoopStartSeconds(start);
    unit->cutfade.setLoopEndSeconds(end);
    unit->cutfade.setFadeTime(fade);
    unit->cutfade.setLoopFlag(loop > 0);


    unit->cutfade.setFadeRec(fadeRec);
    unit->cutfade.setFadePre(fadePre);
    unit->cutfade.setRecRun(recRun > 0);

    if ((trig > 0) && (unit->prevTrig <= 0)) {
        unit->cutfade.cutToStart();
    }
    unit->prevTrig = trig;

    float snd, phi, tr;
    for (int i = 0; i < inNumSamples; ++i) {

        unit->cutfade.setRate(rate[i]);
        unit->cutfade.setRec(rec[i]);
        unit->cutfade.setPre(pre[i]);

        unit->cutfade.nextSample(in[i], &phi, &tr, &snd);
   //     phase_out[i] = phi;

        ///// debug!
        phase_out[i] = unit->cutfade.x;


        trig_out[i] = tr;
        snd_out[i] = snd;

//s        Print("fract phase: %f\n", unit->cutfade.x);
    }
}

PluginLoad(CutFadeVoice) {
    ft = inTable;
    DefineSimpleUnit(CutFadeVoice);
    Print("PluginLoad(CutFadeVoice)\n");
}
