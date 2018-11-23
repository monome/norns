//
// Created by emb on 11/19/18.
//

// hack: faust APIUI should include this, but doesn't
#include <cstring>
#include "faust/gui/APIUI.h"

#include "AudioMain.h"
#include "Commands.h"


using namespace crone;
using std::cout;
using std::endl;

AudioMain::AudioMain() {
    comp.init(48000);

    APIUI &ui = comp.getUi();

    ui.setParamValue(ui.getParamIndex("/StereoCompressor/ratio"), 4.f);
    ui.setParamValue(ui.getParamIndex("/StereoCompressor/threshold"), -20.f);
    ui.setParamValue(ui.getParamIndex("/StereoCompressor/attack"), 0.005f);
    ui.setParamValue(ui.getParamIndex("/StereoCompressor/release"), 0.05f);
    ui.setParamValue(ui.getParamIndex("/StereoCompressor/preGain"), 0.0);
    ui.setParamValue(ui.getParamIndex("/StereoCompressor/postGain"), 4.0);

    cout << " compressor params: " << endl;
    for(int i=0; i<ui.getParamsCount(); ++i) {
        cout << "  " << i << ": " << ui.getParamLabel(i)
        << " (" << ui.getParamAddress(i) << ")"
        << " = " << ui.getParamValue(i) << endl;
    }
}

AudioMain::AudioMain(int sampleRate) {
    comp.init(sampleRate);
}

void AudioMain::processBlock(const float **in_adc, const float **in_ext, float **out, size_t numFrames) {
    Commands::handlePending(this);

    // clear all our internal busses
    clearBusses(numFrames);

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
    bus.ins_in.mixFrom(bus.adc_monitor, numFrames, smoothLevels.monitor);
    bus.ins_in.sumFrom(bus.ext_out, numFrames);
    bus.ins_in.mixFrom(bus.aux_out, numFrames, smoothLevels.aux);

    // apply insert fx
#if 0
    bus.ins_out.sumFrom(bus.ins_in, numFrames);
#else
//    // FIXME: arg.
    float *pin[] = {bus.ins_in.buf[0], bus.ins_in.buf[1]};
    float *pout[] = {bus.ins_out.buf[0], bus.ins_out.buf[1]};
    comp.processBlock(pin, pout, numFrames);
#endif

    // apply insert wet/dry balance
#if 0
    bus.dac_in.sumFrom(bus.ins_out, numFrames);
#else
    bus.dac_in.xfade(bus.ins_in, bus.ins_out, numFrames, smoothLevels.ins_mix);
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
        case Commands::Id::SET_LEVEL_INS_MIX:
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
