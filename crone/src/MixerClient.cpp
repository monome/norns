//
// Created by emb on 11/28/18.
//

#include "MixerClient.h"

using namespace crone;

MixerClient::MixerClient() : Client<6, 6>("crone") {}

void MixerClient::process(jack_nframes_t numFrames) {
    clearBusses(numFrames);

    // mix inputs
    bus.adc_source.setFrom(source[SOURCE_ADC], numFrames, smoothLevels.adc);
    bus.cut_source.setFrom(source[SOURCE_CUT], numFrames, smoothLevels.cut);
    bus.ext_source.setFrom(source[SOURCE_EXT], numFrames, smoothLevels.ext);

    // mix ADC monitor
    bus.adc_monitor.stereoMixFrom(bus.adc_source, numFrames, staticLevels.monitor_mix);
    // copy ADC->ext
    bus.ext_sink.copyFrom(bus.adc_source, numFrames);
    // mix ADC->cut, ext->cut
    bus.cut_sink.sumFrom(bus.adc_source, numFrames);
    bus.cut_sink.mixFrom(bus.ext_source, numFrames, smoothLevels.ext_cut);

    processFx(numFrames);

    bus.dac_sink.mixTo(sink[SinkId::SINK_DAC], numFrames, smoothLevels.dac);
}

void MixerClient::setSampleRate(jack_nframes_t sr) {
    smoothLevels.setSampleRate(sr);
}


void MixerClient::processFx(size_t numFrames) {
    // FIXME: current faust architecture needs this, for some reason :?
    float* pin[2];
    float* pout[2];
    if (!enabled.reverb) { // bypass aux
        bus.aux_out.sumFrom(bus.aux_in, numFrames);
    } else { // process aux
        bus.aux_in.mixFrom(bus.adc_monitor, numFrames, smoothLevels.monitor_aux);
        bus.aux_in.mixFrom(bus.ext_source, numFrames, smoothLevels.ext_aux);
        pin[0] = bus.aux_in.buf[0];
        pin[1] = bus.aux_in.buf[1];
        pout[0] = bus.aux_out.buf[0];
        pout[1] = bus.aux_out.buf[1];
        reverb.processBlock(pin, pout, static_cast<int>(numFrames));
        bus.ins_in.mixFrom(bus.aux_out, numFrames, smoothLevels.aux);
    }

    // mix to insert bus
    bus.ins_in.mixFrom(bus.adc_monitor, numFrames, smoothLevels.monitor);
    bus.ins_in.sumFrom(bus.cut_source, numFrames);
    bus.ins_in.sumFrom(bus.ext_source, numFrames);

    if(!enabled.comp) { // bypass_insert
        bus.dac_sink.sumFrom(bus.ins_in, numFrames);
    } else {
        pin[0] = bus.ins_in.buf[0];
        pin[1] = bus.ins_in.buf[1];
        pout[0] = bus.ins_out.buf[0];
        pout[1] = bus.ins_out.buf[1];
        comp.processBlock(pin, pout, static_cast<int>(numFrames));
        // apply insert wet/dry
        bus.dac_sink.xfade(bus.ins_in, bus.ins_out, numFrames, smoothLevels.ins_mix);
    }
}

void MixerClient::clearBusses(size_t numFrames) {
    // NB: we only need clear busses that are used as mix destinations each block.
    // if a bus is simply copied to, there's no need to clear it.
    bus.cut_sink.clear(numFrames);
    bus.dac_sink.clear(numFrames);
    bus.ins_in.clear(numFrames);
    bus.aux_in.clear(numFrames);
    bus.adc_monitor.clear(numFrames);
}

// state constructors
MixerClient::BusList::BusList() {
    adc_source.clear();
    ext_source.clear();
    ext_sink.clear();
    cut_sink.clear();
    dac_sink.clear();
    ins_in.clear();
    ins_out.clear();
    aux_in.clear();
    aux_out.clear();
    adc_monitor.clear();
}

MixerClient::SmoothLevelList::SmoothLevelList() {
    adc.setTarget(1.f);
    dac.setTarget(1.f);
    ext.setTarget(1.f);
    cut.setTarget(1.f);
    monitor.setTarget(0.f);
    cut_aux.setTarget(0.f);
    ext_aux.setTarget(0.f);
    ext_cut.setTarget(0.f);
    monitor_aux.setTarget(0.f);
    aux.setTarget(0.f);
    ins_mix.setTarget(0.f);
}

void MixerClient::SmoothLevelList::setSampleRate(float sr) {
    adc.setSampleRate(sr);
    dac.setSampleRate(sr);
    ext.setSampleRate(sr);
    cut.setSampleRate(sr);
    monitor.setSampleRate(sr);
    cut_aux.setSampleRate(sr);
    ext_aux.setSampleRate(sr);
    monitor_aux.setSampleRate(sr);
    aux.setSampleRate(sr);
    ins_mix.setSampleRate(sr);
}

MixerClient::StaticLevelList::StaticLevelList() {
    for (auto &f : monitor_mix) {
        f = 0.5f;
    }
}

MixerClient::EnabledList::EnabledList() {
    comp = false;
    reverb = false;
}
