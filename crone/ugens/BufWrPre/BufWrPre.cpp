#include "SC_PlugIn.h"


struct BufWrPre : public Unit
{
  float m_fbufnum;
  SndBuf *m_buf;
};

static InterfaceTable *ft;

static inline bool checkBuffer(Unit * unit, const float * bufData, uint32 bufChannels,
			       uint32 expectedChannels, int inNumSamples)
{
  if (!bufData)
    goto handle_failure;

  if (expectedChannels > bufChannels) {
    if(unit->mWorld->mVerbosity > -1 && !unit->mDone)
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

inline double sc_loop(Unit *unit, double in, double hi, int loop)
{
  // avoid the divide if possible
  if (in >= hi) {
    if (!loop) {
      unit->mDone = true;
      return hi;
    }
    in -= hi;
    if (in < hi) return in;
  } else if (in < 0.) {
    if (!loop) {
      unit->mDone = true;
      return 0.;
    }
    in += hi;
    if (in >= 0.) return in;
  } else return in;

  return in - hi * floor(in/hi);
}

void BufWrPre_next(BufWrPre *unit, int inNumSamples);
void BufWrPre_Ctor(BufWrPre *unit);

void BufWrPre_Ctor(BufWrPre *unit) {
  SETCALC(BufWrPre_next);
  unit->m_fbufnum = -1e9f;
  ClearUnitOutputs(unit, 1);
}

// FIXME: check rates and add BufWrPre_next_aa, _ak?
void BufWrPre_next(BufWrPre *unit, int inNumSamples) {
  float *phasein  = ZIN(1);
  float *prein = ZIN(2);
  int32 loop     = (int32)ZIN0(3);

  GET_BUF;
  uint32 numInputChannels = unit->mNumInputs - 4;
  if (!checkBuffer(unit, bufData, bufChannels, numInputChannels, inNumSamples))
    return;
  
  double loopMax = (double)(bufFrames - (loop ? 0 : 1));

  for (int32 k=0; k<inNumSamples; ++k) {
    double phase = sc_loop((Unit*)unit, ZXP(phasein), loopMax, loop);
    float pre = ZXP(prein);
    int32 iphase = (int32)phase;
    float* table0 = bufData + iphase * bufChannels;
    for (uint32 channel=0; channel<numInputChannels; ++channel)
    table0[channel] = (table0[channel] * pre) + IN(channel+4)[k];
  }
}

PluginLoad(CutFadeLoop)
{
  ft = inTable;
  DefineSimpleUnit(BufWrPre);
}
