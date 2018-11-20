//
// Created by emb on 11/19/18.
//

#include "AudioMain.h"
#include "Commands.h"

using namespace crone;

AudioMain::AudioMain() = default;

void AudioMain::processBlock(const float **in_adc, const float **in_ext, float **out, size_t numFrames) {
    clearBusses(numFrames);
    bus.adc_out.mixFrom(in_adc, numFrames, smoothLevels.adc);
    bus.ext_out.mixFrom(in_ext, numFrames, smoothLevels.ext);

    // TODO: all the things

    bus.dac_in.mixTo(out, numFrames, smoothLevels.dac);
}

void AudioMain::clearBusses(size_t numFrames) {
    bus.adc_out.clear(numFrames);
    bus.ext_out.clear(numFrames);
    bus.dac_in.clear(numFrames);
    bus.ins_in.clear(numFrames);
    bus.ins_out.clear(numFrames);
    bus.aux_in.clear(numFrames);
    bus.aux_out.clear(numFrames);
    bus.adc_monitor.clear(numFrames);
}

AudioMain::BusList::BusList() {
    for (auto *p: { &adc_out, &dac_in, &adc_monitor, &aux_in, &aux_out, &ins_in, &ins_out}) {
        p->clear();
    }
}

AudioMain::SmoothLevelList::SmoothLevelList() {
    for (auto *p : { &adc, &monitor, &monitor_aux, &aux_dac}) {
        p->setTarget(0);
    }
}

AudioMain::StaticLevelList::StaticLevelList() {
    for (auto *p : { &monitor_l_l, &monitor_l_r, &monitor_r_l, &monitor_r_r}) {
        *p = 0;
    }
}


void AudioMain::handleCommand(crone::Commands::CommandPacket *p) {
    switch(p->id) {
        case Commands::Id::SET_LEVEL_ADC:
            smoothLevels.adc.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_DAC:
            smoothLevels.dac.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_EXT:
            smoothLevels.ext.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_EXT_AUX:
            smoothLevels.ext_aux.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_AUX_DAC:
            smoothLevels.aux_dac.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_MONITOR:
            smoothLevels.monitor.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_MONITOR_AUX:
            smoothLevels.monitor_aux.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_INS__MIX:
            smoothLevels.ins_mix.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_MONITOR_MIX:
            switch(p->voice) {
                case 0:
                    staticLevels.monitor_l_l = p->value;
                    break;
                case 1:
                    staticLevels.monitor_l_r = p->value;
                    break;
                case 2:
                    staticLevels.monitor_r_l = p->value;
                    break;
                case 3:
                    staticLevels.monitor_r_r = p->value;
                    break;
                default: ;;
            }
            break;
        default:
            ;;
    }

}
