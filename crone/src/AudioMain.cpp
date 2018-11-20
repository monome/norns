//
// Created by emb on 11/19/18.
//

#include "AudioMain.h"

crone::AudioMain::AudioMain() = default;

void crone::AudioMain::processBlock(const float **in_adc, const float **in_ext, float **out, size_t numFrames) {
    bus.adc_out.mixFrom(in_adc, numFrames, smoothLevels.adc);

    // TODO: all the things

    bus.dac_in.mixTo(out, numFrames, smoothLevels.dac);


}


crone::AudioMain::BusList::BusList() {
    for (auto *p: { &adc_out, &dac_in, &adc_monitor, &aux_in, &aux_out, &ins_in, &ins_out}) {
        p->clear();
    }
}

crone::AudioMain::SmoothLevelList::SmoothLevelList() {
    for (auto *p : { &adc, &monitor, &monitor_aux, &aux_dac}) {
        p->setTarget(0);
    }
}

crone::AudioMain::StaticLevelList::StaticLevelList() {
    for (auto *p : { &monitor_l_l, &monitor_l_r, &monitor_r_l, &monitor_r_r}) {
        *p = 0;
    }
}
