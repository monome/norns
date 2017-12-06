#include "SC_PlugIn.h"

#include "CutFadeLoopLogic.h"

static InterfaceTable *ft;

struct CutFadeLoop : public Unit {

    SndBuf *m_buf;

    CutFadeLoopLogic cutfade;

    // State of the one-pole lowpass filter.
    float s1;
};


static void CutFadeLoop_next(CutFadeLoop *unit, int inNumSamples);
static void CutFadeLoop_Ctor(CutFadeLoop* unit);
static void CutFadeLoop_Dtor(CutFadeLoop* unit);


void CutFadeLoop_Ctor(CutFadeLoop* unit) {
    SETCALC(CutFadeLoop_next);
//
//    unit->maxdelay = IN0(2);
//
//    // To get the buffer size in samples, take the sample rate times the length in seconds.
//    // The buffer size doesn't NEED to be a power of two, but if you're doing a lot of moduloing then it's faster that way.
//    unit->bufsize = NEXTPOWEROFTWO((float)SAMPLERATE * unit->maxdelay);
//    unit->mask = unit->bufsize - 1;
//
//    unit->writephase = 0;
//    unit->s1 = 0;
//
//    // Allocate the buffer. Do NOT use malloc!
//    // SuperCollider provides special real-time-safe allocation and freeing functions.
//    unit->buf = (float*)RTAlloc(unit->mWorld, unit->bufsize * sizeof(float));
//
//    // This check makes sure that RTAlloc succeeded. (It might fail if there's not enough memory.)
//    // If you don't do this check properly then YOU CAN CRASH THE SERVER!
//    // A lot of ugens in core and sc3-plugins fail to do this. Don't follow their example.
//    if (unit->buf == NULL) {
//        // Avoid retaining CutFadeLoop_next as the calculation function.
//        SETCALC(ft->fClearUnitOutputs);
//        ClearUnitOutputs(unit, 1);
//
//        if(unit->mWorld->mVerbosity > -2) {
//            Print("Failed to allocate memory for CutFadeLoop ugen.\n");
//        }
//
//        return;
//    }
//
//    // Fill the buffer with zeros.
//    memset(unit->buf, 0, unit->bufsize * sizeof(float));

    CutFadeLoop_next(unit, 1);
}

// this must be named PluginName_Dtor.
void CutFadeLoop_Dtor(CutFadeLoop* unit) {
    // Free the memory.
    // RTFree(unit->mWorld, unit->buf);
}

void CutFadeLoop_next(CutFadeLoop *unit, int inNumSamples)
{
    // audio-rate input signal
    float *in = IN(0);
    // audio-rate output signal
    float *out = OUT(0);
    // control-rate delay
    float delay = IN0(1);
    // control-rate feedback coefficient
    float fb = IN0(3);
    // control-rate filter coefficient
    float coeff = IN0(4);

//    float* buf = unit->buf;
//    int mask = unit->mask;
//    int writephase = unit->writephase;
//    float s1 = unit->s1;
//
//    // Cap the delay at maxdelay
//    if (delay > unit->maxdelay) {
//        delay = unit->maxdelay;
//    }

//    // Compute the delay in samples and the integer and fractional parts of this delay.
//    float delay_samples = (float)SAMPLERATE * unit->maxdelay;
//    int offset = delay_samples;
//    float frac = delay_samples - offset;
//
//    // Precompute a filter coefficient.
//    float a = 1 - std::abs(coeff);
//
//    for (int i = 0; i < inNumSamples; i++) {
//
//        // Four integer phases into the buffer
//        int phase1 = writephase - offset;
//        int phase2 = phase1 - 1;
//        int phase3 = phase1 - 2;
//        int phase0 = phase1 + 1;
//        float d0 = buf[phase0 & mask];
//        float d1 = buf[phase1 & mask];
//        float d2 = buf[phase2 & mask];
//        float d3 = buf[phase3 & mask];
//        // Use cubic interpolation with the fractional part of the delay in samples
//        float delayed = cubicinterp(frac, d0, d1, d2, d3);
//
//        // Apply lowpass filter and store the state of the filter.
//        float lowpassed = a * delayed + coeff * s1;
//        s1 = lowpassed;
//
//        // Multiply by feedback coefficient and add to input signal.
//        // zapgremlins gets rid of Bad Things like denormals, explosions, etc.
//        out[i] = zapgremlins(in[i] + fb * lowpassed);
//        buf[writephase] = out[i];
//
//        // Here's why the buffer size is a power of two -- otherwise this becomes a much more
//        // expensive modulo operation.
//        writephase = (writephase + 1) & mask;
    }

//    // These two variables were updated and need to be stored back into the state of the UGen.
//    unit->writephase = writephase;
//    unit->s1 = s1;
}


PluginLoad(CutFadeLoop)
{
    ft = inTable;
    // ATTENTION! This has changed!
    // In the previous examples this was DefineSimpleUnit.
    DefineDtorUnit(CutFadeLoop);
}