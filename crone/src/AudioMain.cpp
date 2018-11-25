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
    cut.setSampleRate(static_cast<unsigned int>(sampleRate));
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
    reverb = true;
}

/////////////////////////
/// main process function

void AudioMain::processBlock(const float **in_adc, const float **in_ext, float **out, size_t numFrames) {
    // apply pending param changes
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

    processSoftCut(numFrames);
    processFx(numFrames);

    // apply final output level
    bus.dac_in.mixTo(out, numFrames, smoothLevels.dac);
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


void AudioMain::processSoftCut(size_t numFrames) {
    const float* padc[2] = { bus.adc_out.buf[0], bus.adc_out.buf[1] };
    for(int v=0; v<SOFTCUT_COUNT; ++v) {
        bus.cut_in[v].clear();
        bus.cut_in[v].mixFrom(padc, numFrames, smoothLevels.adc_cut[v][0]);
        bus.cut_in[v].mixFrom(padc+1, numFrames, smoothLevels.adc_cut[v][1]);
        // TODO: feedback
    }
    // TODO: ext in
    // process softcuts
    for(int v=0; v<SOFTCUT_COUNT; ++v) {
        cut.processBlock(v, bus.cut_in[v].buf[0], bus.cut_out[v].buf[0], static_cast<int>(numFrames));
    }
    // mixdown with level/pan
    for(int v=0; v<SOFTCUT_COUNT; ++v) {
        bus.cut_mix.panMixFrom(bus.cut_out[v], numFrames, smoothLevels.cut[v], smoothLevels.cut_pan[v]);
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

            //-- softcut commands
        case Commands::Id::SET_SOFTCUT_RATE:
            cut.setRate(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_LOOP_START:
            cut.setLoopStart(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_LOOP_END:
            cut.setLoopEnd(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_LOOP_FLAG:
            cut.setLoopFlag(p->voice, p->value > 0.f);
            break;
        case Commands::Id::SET_SOFTCUT_FADE_TIME:
            cut.setFadeTime(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_REC_LEVEL:
            cut.setRecLevel(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_PRE_LEVEL:
            cut.setPreLevel(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_REC_FLAG:
            cut.setRecFlag(p->voice, p->value > 0.f);
            break;
        case Commands::Id::SET_SOFTCUT_REC_OFFSET:
            cut.setRecOffset(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_POSITION:
            cut.cutToPos(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_FILTER_FC:
            cut.setFilterFc(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_FILTER_FC_MOD:
            cut.setFilterFcMod(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_FILTER_RQ:
            cut.setFilterRq(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_FILTER_LP:
            cut.setFilterLp(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_FILTER_HP:
            cut.setFilterHp(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_FILTER_BP:
            cut.setFilterBp(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_FILTER_BR:
            cut.setFilterBr(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_FILTER_DRY:
            cut.setFilterDry(p->voice, p->value);
            break;
        // case Commands::Id::SET_SOFTCUT_PRE_FADE_WINDOW:
        //     cut.setPreFadeWindow(p->value);
        //     break;
        // case Commands::Id::SET_SOFTCUT_REC_FADE_DELAY:
        //     cut.setRecFadeDelay(p->value);
        //     break;
        // case Commands::Id::SET_SOFTCUT_PRE_FADE_SHAPE:
        //     cut.setPreFadeShape(p->value);
        //     break;
        // case Commands::Id::SET_SOFTCUT_REC_FADE_SHAPE:
        //     cut.setRecFadeShape(p->value);
        //     break;
        case Commands::Id::SET_SOFTCUT_LEVEL_SLEW_TIME:
            cut.setLevelSlewTime(p->voice, p->value);
            break;
        case Commands::Id::SET_SOFTCUT_RATE_SLEW_TIME:
            cut.setRateSlewTime(p->voice, p->value);
            break;

        default:
            ;;
    }
}

