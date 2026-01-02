// test-local stub for crone::Taper::Vu lookup table symbols
// provides a small linear table to exercise peak meter without the full LUT

#include "Taper.h"

using namespace crone;

// 5-point linear table from 0 to 1, enough for behavior tests
const unsigned int Taper::Vu::ampPosTableSize = 5u;
const float Taper::Vu::ampPosTable[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
