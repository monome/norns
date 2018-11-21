//
// Created by emb on 11/19/18.
//

#include "AudioMain.h"
#include "Commands.h"

using namespace crone;

AudioMain::AudioMain() = default;

void AudioMain::processBlock(const float **in_adc, const float **in_ext, float **out, size_t numFrames) {
    // clear all our internal busses
    clearBusses(numFrames);

    // test..
    return;

    // clear the output
    for(int ch=0; ch<2; ++ch) {
        for (size_t fr=0; fr<numFrames; ++fr) {
            out[ch][fr] = 0.f;
        }
    }

    // apply input levels
    bus.adc_out.mixFrom(in_adc, numFrames, smoothLevels.adc);
    bus.ext_out.mixFrom(in_ext, numFrames, smoothLevels.ext);

    // mix to monitor bus
    bus.adc_monitor.stereoMixFrom(bus.adc_out, numFrames, staticLevels.monitor_mix);

    // mix to aux input
    bus.aux_in.mixFrom(bus.adc_monitor, numFrames, smoothLevels.monitor_aux);
    bus.aux_in.mixFrom(bus.ext_out, numFrames, smoothLevels.ext_aux);

    // apply aux fx
#if 1
    bus.aux_out.sumFrom(bus.aux_in, numFrames);
#else
    // TODO
#endif

    // mix to insert bus
    bus.ins_in.mixFrom(bus.aux_out, numFrames, smoothLevels.aux);

    // apply insert fx
#if 1
    bus.ins_out.sumFrom(bus.ins_in, numFrames);
#else
    // TODO
#endif

    // apply insert wet/dry balance
#if 1
    bus.dac_in.sumFrom(bus.ins_out, numFrames);
#else
    // TODO
#endif

    // apply final output level
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
    for (auto *p : { &adc, &monitor, &monitor_aux, &aux}) {
        p->setTarget(0);
    }
}

AudioMain::StaticLevelList::StaticLevelList() {
    for (auto &f : monitor_mix) {
        f = 0.f;
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
            smoothLevels.aux.setTarget(p->value);
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
            if(p->voice < 0 || p->voice > 3) { return; }
            staticLevels.monitor_mix[p->voice] = p->value;
            break;
        default:
            ;;
    }

}
