#include <utility>
#include <thread>

//
// Created by ezra on 11/4/18.
//

#include "OscInterface.h"
#include "Commands.h"

#include "effects/CompressorParams.h"
#include "effects/ReverbParams.h"
#include "softcut/FadeCurves.h"

using namespace crone;
using softcut::FadeCurves;

/// TODO: softcut trigger/ phase output
/// TODO: softcut soundfile load / write


bool OscInterface::quitFlag;
std::string OscInterface::port;
lo_server_thread OscInterface::st;
std::array<OscInterface::OscMethod, OscInterface::MAX_NUM_METHODS> OscInterface::methods;
unsigned int OscInterface::numMethods = 0;

OscInterface::OscMethod::OscMethod(string p, string f, OscInterface::Handler h)
        : path(std::move(p)), format(std::move(f)), handler(h) {}

void OscInterface::addServerMethods() {
    addServerMethod("/hello", "", [](lo_arg **argv, int argc) {
        (void)argv; (void)argc;
        std::cout << "hello" << std::endl;
    });

    addServerMethod("/goodbye", "", [](lo_arg **argv, int argc) {
        (void)argv; (void)argc;
        std::cout << "goodbye" << std::endl;
        OscInterface::quitFlag = true;
    });

    addServerMethod("/quit", "", [](lo_arg **argv, int argc) {
        (void)argv; (void)argc;
        OscInterface::quitFlag = true;
    });

    addServerMethod("/set/level/adc", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_LEVEL_ADC, argv[0]->f);
    });

    addServerMethod("/set/level/dac", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_LEVEL_DAC, argv[0]->f);
    });

    addServerMethod("/set/level/ext", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_LEVEL_EXT, argv[0]->f);
    });

    addServerMethod("/set/level/ext_aux", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_LEVEL_EXT_AUX, argv[0]->f);
    });

    addServerMethod("/set/level/aux_dac", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_LEVEL_AUX_DAC, argv[0]->f);
    });

    addServerMethod("/set/level/monitor", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_LEVEL_MONITOR, argv[0]->f);
    });

    addServerMethod("/set/level/monitor_mix", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_LEVEL_MONITOR_MIX, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/level/monitor_aux", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_LEVEL_MONITOR_AUX, argv[0]->f);
    });

    addServerMethod("/set/level/ins_mix", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_LEVEL_INS_MIX, argv[0]->f);
    });


    // toggle enabled
    addServerMethod("/set/enabled/compressor", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_ENABLED_COMPRESSOR, argv[0]->f);
    });

    addServerMethod("/set/enabled/reverb", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_ENABLED_REVERB, argv[0]->f);
    });

    //-------------------------
    //-- compressor params

    addServerMethod("/set/param/compressor/ratio", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::RATIO, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/threshold", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::THRESHOLD, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/attack", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::ATTACK, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/release", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::RELEASE, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/gain_pre", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::GAIN_PRE, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/gain_post", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::GAIN_POST, argv[0]->f);
    });


    //--------------------------
    //-- reverb params

    addServerMethod("/set/param/reverb/pre_del", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_REVERB, ReverbParam::PRE_DEL, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/lf_fc", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_REVERB, ReverbParam::LF_FC, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/low_rt60", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_REVERB, ReverbParam::LOW_RT60, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/mid_rt60", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_REVERB, ReverbParam::MID_RT60, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/hf_damp", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_REVERB, ReverbParam::HF_DAMP, argv[0]->f);
    });

    //--------------------------------
    //-- softcut params
    addServerMethod("/set/param/softcut/rate", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_RATE, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/loop_start", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_LOOP_START, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/loop_end", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_LOOP_END, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/loop_flag", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_LOOP_FLAG, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/fade_time", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_FADE_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/rec_level", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_REC_LEVEL, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/pre_level", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_PRE_LEVEL, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/rec_flag", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_REC_FLAG, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/rec_offset", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_REC_OFFSET, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/position", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_POSITION, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/filter_fc", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_FILTER_FC, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/filter_fc_mod", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_FILTER_FC_MOD, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/filter_rq", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_FILTER_RQ, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/filter_lp", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_FILTER_LP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/filter_hp", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_FILTER_HP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/filter_bp", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_FILTER_BP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/filter_br", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_FILTER_BR, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/filter_dry", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_FILTER_DRY, argv[0]->i, argv[1]->f);
    });


    addServerMethod("/set/param/softcut/pre_fade_window", "if", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        float x = argv[0]->f;
        auto t = std::thread([x] {
            FadeCurves::setPreWindowRatio(x);
        });
        t.detach();
    });

    addServerMethod("/set/param/softcut/rec_fade_delay", "if", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        float x = argv[0]->f;
        auto t = std::thread([x] {
            FadeCurves::setRecDelayRatio(x);
        });
        t.detach();
    });

    addServerMethod("/set/param/softcut/pre_fade_shape", "if", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        float x = argv[0]->f;
        auto t = std::thread([x] {
            FadeCurves::setPreShape(static_cast<FadeCurves::Shape>(x));
        });
        t.detach();
    });

    addServerMethod("/set/param/softcut/rec_fade_shape", "if", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        float x = argv[0]->f;
        auto t = std::thread([x] {
            FadeCurves::setRecShape(static_cast<FadeCurves::Shape>(x));
        });
        t.detach();
    });

    addServerMethod("/set/param/softcut/level_slew_time", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_LEVEL_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/softcut/rate_slew_time", "if", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_SOFTCUT_RATE_SLEW_TIME, argv[0]->i, argv[1]->f);
    });


}

