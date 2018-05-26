#include "SC_PlugIn.h"

static InterfaceTable *ft;

struct TrigPhasor : public Unit {
    float prevReset;
    float level;
};

static void TrigPhasor_next(TrigPhasor* unit, int inNumSamples);
static void TrigPhasor_Ctor(TrigPhasor* unit);

void TrigPhasor_Ctor(TrigPhasor* unit) {
    unit->prevReset = 0;
    unit->level = 0;

    SETCALC(TrigPhasor_next);
    TrigPhasor_next(unit, 1);
}

void TrigPhasor_next(TrigPhasor* unit, int inNumSamples) {
    float *reset    = IN(0);
    float *resetPos = IN(1);
    float *start    = IN(2);
    float *end      = IN(3);
    float *step     = IN(4);

    float *out      = OUT(0);
    float *outTrig  = OUT(1);

    for (int i = 0; i < inNumSamples; i++) {
        float slope = end[i] - start[i];

        if (unit->prevReset <= 0 && reset[i] > 0) {
            unit->level = resetPos[i];
        } else {
            unit->level += copysignf(step[i], slope);
        }

        if (slope <= 0) {
            if (unit->level < end[i]) {
                outTrig[i] = 1;
                unit->level = start[i];
            } else {
                outTrig[i] = 0;
            }
        } else {
            if (unit->level > end[i]) {
                outTrig[i] = 1;
                unit->level = start[i];
            } else {
                outTrig[i] = 0;
            }
        }

        out[i] = unit->level;
        unit->prevReset = reset[i];
    }
}

PluginLoad(TrigPhasorUGens) {
    ft = inTable;
    DefineSimpleUnit(TrigPhasor);
}
