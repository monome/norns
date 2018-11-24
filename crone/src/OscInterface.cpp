#include <utility>

//
// Created by ezra on 11/4/18.
//

#include "OscInterface.h"
#include "Commands.h"

#include "effects/CompressorParams.h"
#include "effects/ReverbParams.h"

using namespace crone;


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


    //-------------------------
    //-- compressor params

    addServerMethod("/set/param/Compressor/ratio", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::RATIO, argv[0]->f);
    });

    addServerMethod("/set/param/Compressor/threshold", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::THRESHOLD, argv[0]->f);
    });

    addServerMethod("/set/param/Compressor/attack", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::ATTACK, argv[0]->f);
    });

    addServerMethod("/set/param/Compressor/release", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::RELEASE, argv[0]->f);
    });

    addServerMethod("/set/param/Compressor/gain_pre", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::GAIN_PRE, argv[0]->f);
    });

    addServerMethod("/set/param/Compressor/gain_post", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::GAIN_POST, argv[0]->f);
    });


    //--------------------------
    //-- reverb params

    addServerMethod("/set/param/Reverb/pre_del", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_REVERB, ReverbParam::PRE_DEL, argv[0]->f);
    });

    addServerMethod("/set/param/Reverb/lf_fc", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_REVERB, ReverbParam::LF_FC, argv[0]->f);
    });

    addServerMethod("/set/param/Reverb/low_rt60", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_REVERB, ReverbParam::LOW_RT60, argv[0]->f);
    });

    addServerMethod("/set/param/Reverb/mid_rt60", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_REVERB, ReverbParam::MID_RT60, argv[0]->f);
    });

    addServerMethod("/set/param/Reverb/hf_damp", "f", [](lo_arg **argv, int argc) {
        if(argc<2) { return; }
        Commands::post(Commands::Id::SET_PARAM_REVERB, ReverbParam::HF_DAMP, argv[0]->f);
    });



}

