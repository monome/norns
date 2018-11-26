//
// Created by emb on 11/19/18.
//

#include <cstring> // hack: faust APIUI should include, but doesn't
#include "faust/gui/APIUI.h"

#include "AudioMain.h"
#include "Commands.h"
#include "Evil.h"

using namespace crone;
using std::cout;
using std::endl;

AudioMain::AudioMain() {
    this->init(48000);
}

AudioMain::AudioMain(int sampleRate) {
    this->init(sampleRate);
}


void AudioMain::init(int sampleRate) {

    comp.init(sampleRate);
    reverb.init(sampleRate);
    reverb.init(sampleRate);
    cw.cut.setSampleRate(static_cast<unsigned int>(sampleRate));
    setDefaultParams();
}
// state constructors
AudioMain::BusList::BusList() {
    for (auto *p: { &adc_out, &dac_in, &adc_monitor, &aux_in, &aux_out, &ins_in, &ins_out}) {
        p->clear();
    }
}

AudioMain::SmoothLevelList::SmoothLevelList() {
    adc.setTarget(1.f);
    dac.setTarget(1.f);
    ext.setTarget(1.f);
    monitor.setTarget(0.f);
    ext_aux.setTarget(0.f);
    monitor_aux.setTarget(0.f);
    ins_mix.setTarget(0.f);
}

AudioMain::StaticLevelList::StaticLevelList() {
    for (auto &f : monitor_mix) {
        f = 0.5f;
    }
}

AudioMain::EnabledList::EnabledList() {
    comp = false;
    reverb = false;
}

/////////////////////////
/// main process function

void AudioMain::processBlock(const float **in_adc, const float **in_ext, float **out, size_t numFrames) {
    // apply pending param changes
    Commands::handlePending(this);
    // clear internal busses
    clearBusses(numFrames);
    // clear the output
    for(int ch=0; ch<2; ++ch) {
        for (size_t fr=0; fr<numFrames; ++fr) {
            out[ch][fr] = 0.f;
        }
    }


    // mix input ports to input busses, applying levels
    bus.adc_out.mixFrom(in_adc, numFrames, smoothLevels.adc);
    bus.ext_out.mixFrom(in_ext, numFrames, smoothLevels.ext);

    /// -- simplest way of dividing softcut work:
    /// -- 1) mix all of SC I/O using using data from last buffer
    mixCutInputs(numFrames);
    mixCutOutputs(numFrames);

    /// -- 2) start softcut work
    cw.startProcess(numFrames);

    /// -- 3) continue mixing and processing other fx
    // mix to monitor bus
    bus.adc_monitor.stereoMixFrom(bus.adc_out, numFrames, staticLevels.monitor_mix);

    processFx(numFrames);

    // apply final output level
    bus.dac_in.mixTo(out, numFrames, smoothLevels.dac);

    /// -- 4) wait for softcut worker to finish
    /// FIXME: shouldn't really be necessary,
    /// but without it underruns might put very bad stuff in the audio buffer.
    // cw.waitForDone();
}

void AudioMain::processFx(size_t numFrames)  {

    // FIXME: current faust architecture needs this
    float* pin[2];
    float* pout[2];
    if (!enabled.reverb) { // bypass aux
        bus.aux_out.sumFrom(bus.aux_in, numFrames);
    } else { // process aux
        // mix to aux input
        bus.aux_in.mixFrom(bus.adc_monitor, numFrames, smoothLevels.monitor_aux);
        bus.aux_in.mixFrom(bus.ext_out, numFrames, smoothLevels.ext_aux);
        // FIXME: arg!
        pin[0] = bus.aux_in.buf[0];
        pin[1] = bus.aux_in.buf[1];
        pout[0] = bus.aux_out.buf[0];
        pout[1] = bus.aux_out.buf[1];
        reverb.processBlock(pin, pout, static_cast<int>(numFrames));
        bus.ins_in.mixFrom(bus.aux_out, numFrames, smoothLevels.aux);
    }

    // mix to insert bus
    bus.ins_in.mixFrom(bus.adc_monitor, numFrames, smoothLevels.monitor);
    bus.ins_in.sumFrom(bus.ext_out, numFrames);

    if(!enabled.comp) { // bypass_insert
        bus.dac_in.sumFrom(bus.ins_in, numFrames);
    } else { // process insert
        // FIXME: arg!
        pin[0] = bus.ins_in.buf[0];
        pin[1] = bus.ins_in.buf[1];
        pout[0] = bus.ins_out.buf[0];
        pout[1] = bus.ins_out.buf[1];
        comp.processBlock(pin, pout, static_cast<int>(numFrames));
        // apply insert wet/dry
        bus.dac_in.xfade(bus.ins_in, bus.ins_out, numFrames, smoothLevels.ins_mix);
    }
}


void AudioMain::mixCutInputs(size_t numFrames) {
    for (auto &b: cw.cut_in) { b.clear(numFrames); };
    // mix adc in
    const float *pin[2] = {bus.adc_out.buf[0], bus.adc_out.buf[1]};
    for (int v = 0; v < SOFTCUT_COUNT; ++v) {
        if (!cw.enabled[v]) { continue; }
        cw.cut_in[v].clear();
        cw.cut_in[v].mixFrom(pin, numFrames, smoothLevels.adc_cut[v][0]);
        cw.cut_in[v].mixFrom(pin + 1, numFrames, smoothLevels.adc_cut[v][1]);
#if 0 // FIXME this is messed up?
        // mix feedback
        for(int w=0; w<SOFTCUT_COUNT; ++w) {
            bus.cut_in[v].mixFrom(bus.cut_out[w], numFrames, smoothLevels.cut_fb[v][w]);
        }
#endif
    }
    // mix ext in
    pin[0] = bus.ext_out.buf[0];
    pin[1] = bus.ext_out.buf[1];
    for (int v = 0; v < SOFTCUT_COUNT; ++v) {
        if (!cw.enabled[v]) { continue; }
        cw.cut_in[v].mixFrom(pin, numFrames, smoothLevels.ext_cut[v][0]);
        cw.cut_in[v].mixFrom(pin + 1, numFrames, smoothLevels.ext_cut[v][1]);
    }
//    // process softcuts (overwrites output bus)
//    for(int v=0; v<SOFTCUT_COUNT; ++v) {
//        if(!enabled.cut[v]) { continue; }
//        cut.processBlock(v, bus.cut_in[v].buf[0], bus.cut_out[v].buf[0], static_cast<int>(numFrames));
//    }

}

void AudioMain::mixCutOutputs(size_t numFrames) {
    // mixdown with level/pan
    for(int v=0; v<SOFTCUT_COUNT; ++v) {
        if(!cw.enabled[v]) { continue; }
        bus.cut_mix.panMixFrom(cw.cut_out[v], numFrames, smoothLevels.cut[v], smoothLevels.cut_pan[v]);
    }
    // mix to output/send
    bus.aux_in.mixFrom(bus.cut_mix, numFrames, smoothLevels.cut_aux);
    bus.ins_in.sumFrom(bus.cut_mix, numFrames);
}


void AudioMain::setDefaultParams() {

    APIUI *ui;
    ui = &comp.getUi();
    ui->setParamValue(ui->getParamIndex("/StereoCompressor/ratio"), 4.f);
    ui->setParamValue(ui->getParamIndex("/StereoCompressor/threshold"), -20.f);
    ui->setParamValue(ui->getParamIndex("/StereoCompressor/attack"), 0.005f);
    ui->setParamValue(ui->getParamIndex("/StereoCompressor/release"), 0.05f);
    ui->setParamValue(ui->getParamIndex("/StereoCompressor/gain_pre"), 0.0);
    ui->setParamValue(ui->getParamIndex("/StereoCompressor/gain_post"), 4.0);

    ui = &reverb.getUi();
    ui->setParamValue(ui->getParamIndex("/ZitaReverb/pre_del"), 47);
    ui->setParamValue(ui->getParamIndex("/ZitaReverb/lf_fc"), 666);
    ui->setParamValue(ui->getParamIndex("/ZitaReverb/low_rt60"), 3.33);
    ui->setParamValue(ui->getParamIndex("/ZitaReverb/mid_rt60"), 4.44);
    ui->setParamValue(ui->getParamIndex("/ZitaReverb/hf_damp"), 5555);

    if(0) {
        std::vector<Evil::FaustModule> mods;
        Evil::FaustModule mcomp("Compressor", &comp.getUi());
        mods.push_back(mcomp);
        Evil::FaustModule mverb("Reverb", &reverb.getUi());
        mods.push_back(mverb);
        Evil::DO_EVIL(mods); /// ayyehehee
    }
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
    bus.cut_mix.clear(numFrames);
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
        case Commands::Id::SET_PARAM_REVERB:
            reverb.getUi().setParamValue(p->voice, p->value);
            break;
        case Commands::Id::SET_PARAM_COMPRESSOR:
            comp.getUi().setParamValue(p->voice, p->value);
            break;
        case Commands::Id::SET_ENABLED_REVERB:
            enabled.reverb = p->value > 0.f;
            break;
        case Commands::Id::SET_ENABLED_COMPRESSOR:
            enabled.comp = p->value > 0.f;
            break;

            //-- softcut routing
        case Commands::Id::SET_ENABLED_CUT:
            cw.enabled[p->voice] = p->value > 0.f;
            break;
        case Commands::Id::SET_LEVEL_CUT:
            smoothLevels.cut[p->voice].setTarget(p->value);
            break;;
        case Commands::Id::SET_PAN_CUT:
            smoothLevels.cut_pan[p->voice].setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_CUT_AUX:
            smoothLevels.cut_aux.setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_ADC_0_CUT:
            smoothLevels.adc_cut[p->voice][0].setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_ADC_1_CUT:
            smoothLevels.adc_cut[p->voice][1].setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_EXT_0_CUT:
            smoothLevels.ext_cut[p->voice][0].setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_EXT_1_CUT:
            smoothLevels.ext_cut[p->voice][1].setTarget(p->value);
            break;

            //-- softcut commands
        case Commands::Id::SET_CUT_RATE:
            cw.cut.setRate(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_LOOP_START:
            cw.cut.setLoopStart(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_LOOP_END:
            cw.cut.setLoopEnd(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_LOOP_FLAG:
            cw.cut.setLoopFlag(p->voice, p->value > 0.f);
            break;
        case Commands::Id::SET_CUT_FADE_TIME:
            cw.cut.setFadeTime(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_REC_LEVEL:
            cw.cut.setRecLevel(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_PRE_LEVEL:
            cw.cut.setPreLevel(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_REC_FLAG:
            cw.cut.setRecFlag(p->voice, p->value > 0.f);
            break;
        case Commands::Id::SET_CUT_REC_OFFSET:
            cw.cut.setRecOffset(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_POSITION:
            cw.cut.cutToPos(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_FC:
            cw.cut.setFilterFc(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_FC_MOD:
            cw.cut.setFilterFcMod(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_RQ:
            cw.cut.setFilterRq(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_LP:
            cw.cut.setFilterLp(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_HP:
            cw.cut.setFilterHp(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_BP:
            cw.cut.setFilterBp(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_BR:
            cw.cut.setFilterBr(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_DRY:
            cw.cut.setFilterDry(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_LEVEL_SLEW_TIME:
            cw.cut.setLevelSlewTime(p->voice, p->value);
            break;
        case Commands::Id::SET_CUT_RATE_SLEW_TIME:
            cw.cut.setRateSlewTime(p->voice, p->value);
            break;

        default:
            ;;
    }
}

