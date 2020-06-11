//
// Created by emb on 11/28/18.
//

#include "MixerClient.h"
#include "Commands.h"

#include "effects/CompressorParams.h"
#include "effects/ReverbParams.h"

#include "Tape.h"

using namespace crone;

MixerClient::MixerClient() : Client<6, 6>("crone") {}

void MixerClient::process(jack_nframes_t numFrames) {
    Commands::mixerCommands.handlePending(this);

    // copy inputs
    bus.adc_source.setFrom(source[SourceAdc], numFrames, smoothLevels.adc);
    bus.cut_source.setFrom(source[SourceCut], numFrames, smoothLevels.cut);
    bus.ext_source.setFrom(source[SourceExt], numFrames, smoothLevels.ext);

    // mix ADC monitor
    bus.adc_monitor.clear(numFrames);
    bus.adc_monitor.stereoMixFrom(bus.adc_source, numFrames, staticLevels.monitor_mix);

    // copy ADC->ext
    bus.ext_sink.copyFrom(bus.adc_source, numFrames);

    // mix ADC->cut, ext->cut, tape->cut
    bus.cut_sink.clear(numFrames);
    bus.cut_sink.mixFrom(bus.adc_source, numFrames, smoothLevels.adc_cut);
    bus.cut_sink.mixFrom(bus.ext_source, numFrames, smoothLevels.ext_cut);
    bus.cut_sink.mixFrom(bus.tape, numFrames, smoothLevels.tape_cut);


    bus.ins_in.clear(numFrames);
    // process tape playback
    if (tape.isReading()) {
        bus.tape.clear();
        // FIXME: another stupid pointer array.
        float *dst[2] = {static_cast<float*>(bus.tape.buf[0]), static_cast<float*>(bus.tape.buf[1])};
        tape.reader.process(dst, numFrames);
        bus.tape.applyGain(numFrames, smoothLevels.tape);
        bus.ins_in.addFrom(bus.tape, numFrames);
    }

    processFx(numFrames);

    // perform  output
    bus.dac_sink.mixTo(sink[SinkId::SinkDac], numFrames, smoothLevels.dac);
    bus.cut_sink.copyTo(sink[SinkId::SinkCut], numFrames);
    bus.ext_sink.copyTo(sink[SinkId::SinkExt], numFrames);

    // process tape record
    if (tape.isWriting()) {
        const float *src[2] = {(const float *) bus.dac_sink.buf[0], (const float *) bus.dac_sink.buf[1]};
        tape.writer.process(src, numFrames);
    }

    // update peak meters
    inPeak[0].update(bus.adc_source.buf[0], numFrames);
    inPeak[1].update(bus.adc_source.buf[1], numFrames);
    outPeak[0].update(sink[SinkId::SinkDac][0], numFrames);
    outPeak[1].update(sink[SinkId::SinkDac][1] , numFrames);
}

void MixerClient::setSampleRate(jack_nframes_t sr) {
    smoothLevels.setSampleRate(sr);
    comp.init(sr);
    reverb.init(sr);
    setFxDefaults();
}

void MixerClient::processFx(size_t numFrames) {
    // FIXME: current faust architecture needs stupid pointer arrays.
    float* pin[2];
    float* pout[2];
    if (!enabled.reverb) { // bypass aux
        bus.aux_out.clear(numFrames);
        bus.aux_out.addFrom(bus.aux_in, numFrames);
    } else { // process aux
        bus.aux_in.clear(numFrames);
        bus.aux_in.mixFrom(bus.adc_monitor, numFrames, smoothLevels.monitor_aux);
        bus.aux_in.mixFrom(bus.cut_source, numFrames, smoothLevels.cut_aux);
        bus.aux_in.mixFrom(bus.ext_source, numFrames, smoothLevels.ext_aux);
        bus.aux_in.mixFrom(bus.tape, numFrames, smoothLevels.tape_aux);
        pin[0] = bus.aux_in.buf[0];
        pin[1] = bus.aux_in.buf[1];
        pout[0] = bus.aux_out.buf[0];
        pout[1] = bus.aux_out.buf[1];
        reverb.processBlock(pin, pout, static_cast<int>(numFrames));
        bus.ins_in.mixFrom(bus.aux_out, numFrames, smoothLevels.aux);
    }

    // mix to insert bus
    bus.ins_in.mixFrom(bus.adc_monitor, numFrames, smoothLevels.monitor);
    bus.ins_in.addFrom(bus.cut_source, numFrames);
    bus.ins_in.addFrom(bus.ext_source, numFrames);

    bus.dac_sink.clear(numFrames);
    if(!enabled.comp) { // bypass_insert
        bus.dac_sink.addFrom(bus.ins_in, numFrames);
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

void MixerClient::handleCommand(Commands::CommandPacket *p) {
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
	case Commands::Id::SET_LEVEL_CUT_MASTER:
            smoothLevels.cut.setTarget(p->value);
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
            if(p->idx_0 < 0 || p->idx_0 > 3) { return; }
            staticLevels.monitor_mix[p->idx_0] = p->value;
            break;
        case Commands::Id::SET_LEVEL_TAPE:
            smoothLevels.tape.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_TAPE_AUX:
            smoothLevels.tape_aux.setTarget(p->value);
            break;
        case Commands::Id::SET_PARAM_REVERB:
            reverb.getUi().setParamValue(p->idx_0, p->value);
            break;
        case Commands::Id::SET_PARAM_COMPRESSOR:
            comp.getUi().setParamValue(p->idx_0, p->value);
            break;
        case Commands::Id::SET_ENABLED_REVERB:
            enabled.reverb = p->value > 0.f;
            break;
        case Commands::Id::SET_ENABLED_COMPRESSOR:
            enabled.comp = p->value > 0.f;
            break;

            //-- softcut routing
        case Commands::Id::SET_LEVEL_ADC_CUT:
            smoothLevels.adc_cut.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_EXT_CUT:
            smoothLevels.ext_cut.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_TAPE_CUT:
            smoothLevels.tape_cut.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_CUT_AUX:
            smoothLevels.cut_aux.setTarget(p->value);
            break;
        default:
            ;;
    }
}


// state constructors
MixerClient::BusList::BusList() {
    adc_source.clear();
    cut_source.clear();
    ext_source.clear();
    ext_sink.clear();
    cut_sink.clear();
    dac_sink.clear();
    ins_in.clear();
    ins_out.clear();
    aux_in.clear();
    aux_out.clear();
    adc_monitor.clear();
    tape.clear();

}

MixerClient::SmoothLevelList::SmoothLevelList() {
    adc.setTarget(1.f);
    dac.setTarget(1.f);
    ext.setTarget(1.f);
    cut.setTarget(1.f);
    monitor.setTarget(0.f);
    tape.setTarget(0.f);
    adc_cut.setTarget(0.f);
    ext_cut.setTarget(0.f);
    tape_cut.setTarget(0.f);
    monitor_aux.setTarget(0.f);
    cut_aux.setTarget(0.f);
    ext_aux.setTarget(0.f);
    tape_aux.setTarget(0.f);
    aux.setTarget(0.f);
    ins_mix.setTarget(0.f);
}

void MixerClient::SmoothLevelList::setSampleRate(float sr) {
    adc.setSampleRate(sr);
    dac.setSampleRate(sr);
    ext.setSampleRate(sr);
    cut.setSampleRate(sr);
    monitor.setSampleRate(sr);
    tape.setSampleRate(sr);
    adc_cut.setSampleRate(sr);
    ext_cut.setSampleRate(sr);
    tape_cut.setSampleRate(sr);
    monitor_aux.setSampleRate(sr);
    cut_aux.setSampleRate(sr);
    ext_aux.setSampleRate(sr);
    tape_aux.setSampleRate(sr);
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


void MixerClient::setFxDefaults() {
  comp.getUi().setParamValue(CompressorParam::RATIO, 4.0);
  comp.getUi().setParamValue(CompressorParam::THRESHOLD, -12.f);
  comp.getUi().setParamValue(CompressorParam::ATTACK, 0.005);
  comp.getUi().setParamValue(CompressorParam::RELEASE, 0.08);
  comp.getUi().setParamValue(CompressorParam::GAIN_PRE, 0.0);
  comp.getUi().setParamValue(CompressorParam::GAIN_POST, 4.0);
  reverb.getUi().setParamValue(ReverbParam::PRE_DEL, 20);
  reverb.getUi().setParamValue(ReverbParam::LF_FC, 555);
  reverb.getUi().setParamValue(ReverbParam::LOW_RT60, 4.7);
  reverb.getUi().setParamValue(ReverbParam::MID_RT60, 2.3);
  reverb.getUi().setParamValue(ReverbParam::HF_DAMP, 6666);
}
