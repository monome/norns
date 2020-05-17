

//
// Created by ezra on 11/4/18.
//

#include <utility>
#include <thread>
#include <boost/format.hpp>

#include "effects/CompressorParams.h"
#include "effects/ReverbParams.h"

#include "BufDiskWorker.h"
#include "Commands.h"
#include "OscInterface.h"

// for testing
# include "dsp-kit/easing.hpp"

using namespace crone;

bool OscInterface::quitFlag;

std::string OscInterface::port;
lo_server_thread OscInterface::st;
lo_address OscInterface::matronAddress;

std::array<OscInterface::OscMethod, OscInterface::MaxNumMethods> OscInterface::methods;
unsigned int OscInterface::numMethods = 0;

std::unique_ptr<Poll> OscInterface::vuPoll;
std::unique_ptr<Poll> OscInterface::phasePoll;
MixerClient *OscInterface::mixerClient;
SoftcutClient *OscInterface::softCutClient;

OscInterface::OscMethod::OscMethod(string p, string f, OscInterface::Handler h)
        : path(std::move(p)), format(std::move(f)), handler(h) {}


void OscInterface::init(MixerClient *m, SoftcutClient *sc) {
    mixerClient = m;
    softCutClient = sc;
    
    quitFlag = false;
    // FIXME: should get port configs from program args or elsewhere
    port = "9999";
#if 1
    matronAddress = lo_address_new("127.0.0.1", "8888");
#else  // testing with SC
    matronAddress = lo_address_new("127.0.0.1", "57120");
#endif

    st = lo_server_thread_new(port.c_str(), handleLoError);
    
    initServerMethods();
    initPolls();
    
    //--- TODO: softcut trigger poll?

    //--- TODO: tape poll?

    lo_server_thread_start(st);
}

void OscInterface::initPolls() {    
    //--- VU poll
    vuPoll = std::make_unique<Poll>("vu");
    vuPoll->setCallback([](const char *path) {
        char l[4];

        l[0] = (uint8_t) (64 * mixerClient->getInputPeakPos(0));
        l[1] = (uint8_t) (64 * mixerClient->getInputPeakPos(1));
        l[2] = (uint8_t) (64 * mixerClient->getOutputPeakPos(0));
        l[3] = (uint8_t) (64 * mixerClient->getOutputPeakPos(1));

        lo_blob bl = lo_blob_new(sizeof(l), l);
        lo_send(matronAddress, path, "b", bl);
    });
    vuPoll->setPeriod(50);

    //--- softcut phase poll
    phasePoll = std::make_unique<Poll>("softcut/phase");
    phasePoll->setCallback([](const char *path) {
        for (int i = 0; i < softCutClient->getNumVoices(); ++i) {
            if (softCutClient->checkVoiceQuantPhase(i)) {
                lo_send(matronAddress, path, "if", i, softCutClient->getQuantPhase(i));
            }
        }
    });
    phasePoll->setPeriod(1);
}

void OscInterface::addServerMethod(const char *path, const char *format, Handler handler) {
    OscMethod m(path, format, handler);
    methods[numMethods] = m;
    lo_server_thread_add_method(st, path, format,
                                [](const char *path,
                                   const char *types,
                                   lo_arg **argv,
                                   int argc,
                                   lo_message msg,
                                   void *data)
                                        -> int {
                                    (void) path;
                                    (void) types;
                                    (void) msg;
                                    auto pm = static_cast<OscMethod *>(data);
                                    //std::cerr << "osc rx: " << path << std::endl;
                                    pm->handler(argv, argc);
                                    return 0;
                                }, &(methods[numMethods]));
    numMethods++;
}


void OscInterface::initServerMethods() {
    addServerMethod("/hello", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        std::cout << "hello" << std::endl;
    });

    addServerMethod("/goodbye", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        std::cout << "goodbye" << std::endl;
        OscInterface::quitFlag = true;
    });

    addServerMethod("/quit", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        OscInterface::quitFlag = true;
    });


    //---------------------------
    //--- mixer polls

    addServerMethod("/poll/start/vu", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        vuPoll->start();
    });

    addServerMethod("/poll/stop/vu", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        vuPoll->stop();
    });


    ////////////////////////////////
    /// FIXME: many of these methods are trivial setters;
    // they could simply be structured around atomic fields instead of requiring the Command queue.

    //--------------------------
    //--- levels
    addServerMethod("/set/level/adc", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_ADC, argv[0]->f);
    });

    addServerMethod("/set/level/dac", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_DAC, argv[0]->f);
    });

    addServerMethod("/set/level/ext", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_EXT, argv[0]->f);
    });

    addServerMethod("/set/level/cut_master", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_CUT_MASTER, argv[0]->f);
    });


    addServerMethod("/set/level/ext_rev", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_EXT_AUX, argv[0]->f);
    });

    addServerMethod("/set/level/rev_dac", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_AUX_DAC, argv[0]->f);
    });

    addServerMethod("/set/level/monitor", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_MONITOR, argv[0]->f);
    });

    addServerMethod("/set/level/monitor_mix", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_MONITOR_MIX, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/level/monitor_rev", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_MONITOR_AUX, argv[0]->f);
    });

    addServerMethod("/set/level/compressor_mix", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_INS_MIX, argv[0]->f);
    });


    // toggle enabled
    addServerMethod("/set/enabled/compressor", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_ENABLED_COMPRESSOR, argv[0]->f);
    });

    addServerMethod("/set/enabled/reverb", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_ENABLED_REVERB, argv[0]->f);
    });

    //-------------------------
    //-- compressor params

    addServerMethod("/set/param/compressor/ratio", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::RATIO, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/threshold", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::THRESHOLD, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/attack", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::ATTACK, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/release", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::RELEASE, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/gain_pre", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::GAIN_PRE, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/gain_post", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::GAIN_POST, argv[0]->f);
    });


    //--------------------------
    //-- reverb params

    addServerMethod("/set/param/reverb/pre_del", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_REVERB, ReverbParam::PRE_DEL, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/lf_fc", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_REVERB, ReverbParam::LF_FC, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/low_rt60", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_REVERB, ReverbParam::LOW_RT60, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/mid_rt60", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_REVERB, ReverbParam::MID_RT60, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/hf_damp", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_REVERB, ReverbParam::HF_DAMP, argv[0]->f);
    });


    //--------------------------------
    //-- softcut routing

    addServerMethod("/set/enabled/cut", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_ENABLED_CUT, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/level/cut", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_LEVEL, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/pan/cut", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_PAN, argv[0]->i, argv[1]->f);
    });


    addServerMethod("/set/level/adc_cut", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_ADC_CUT, argv[0]->f);
    });

    addServerMethod("/set/level/ext_cut", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_EXT_CUT, argv[0]->f);
    });

    addServerMethod("/set/level/tape_cut", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_TAPE_CUT, argv[0]->f);
    });

    addServerMethod("/set/level/cut_rev", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_CUT_AUX, argv[0]->f);
    });


    //--- NB: these are handled by the softcut command queue,
    // because their corresponding mix points are processed by the softcut client.

    // input channel -> voice levels
    addServerMethod("/set/level/in_cut", "iif", [](lo_arg **argv, int argc) {
        if (argc < 3) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_LEVEL_IN_CUT, argv[0]->i, argv[1]->i, argv[2]->f);
    });


    // voice ->  voice levels
    addServerMethod("/set/level/cut_cut", "iif", [](lo_arg **argv, int argc) {
        if (argc < 3) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_LEVEL_CUT_CUT, argv[0]->i, argv[1]->i, argv[2]->f);
    });


    //--------------------------------
    //-- softcut params

    addServerMethod("/set/param/cut/rate", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_RATE, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/loop_start", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_LOOP_START, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/loop_end", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_LOOP_END, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/loop_flag", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_LOOP_ENABLED, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/fade_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_FADE_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/rec_level", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_REC_LEVEL, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_level", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_PRE_LEVEL, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/rec_flag", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_REC_ENABLED, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/play_flag", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_PLAY_ENABLED, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/rec_offset", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_REC_OFFSET, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/position", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_POSITION, argv[0]->i, argv[1]->f);
    });

    // --- input filter
    addServerMethod("/set/param/cut/pre_filter_fc", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_PRE_FILTER_FC, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_filter_fc_mod", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_PRE_FILTER_FC_MOD, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_filter_q", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_PRE_FILTER_Q, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_filter_enabled", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_PRE_FILTER_ENABLED, argv[0]->i, argv[1]->f);
    });

    // --- output filter
    addServerMethod("/set/param/cut/post_filter_enabled", "if", [
    ](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_POST_FILTER_ENABLED, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_fc", "if", [
    ](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_POST_FILTER_FC, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_rq", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_POST_FILTER_RQ, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_lp", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_POST_FILTER_LP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_hp", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_POST_FILTER_HP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_bp", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_POST_FILTER_BP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_br", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_POST_FILTER_BR, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_dry", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_POST_FILTER_DRY, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/voice_sync", "iif", [](lo_arg **argv, int argc) {
        if (argc < 3) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_SYNC, argv[0]->i, argv[1]->i, argv[2]->f);
    });

    //-- slew times and shapes
    
    addServerMethod("/set/param/cut/level_slew_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_LEVEL_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pan_slew_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_PAN_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/recpre_slew_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_RECPRE_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/rate_slew_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_RATE_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/rate_slew_shape", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_RATE_SLEW_SHAPE, argv[0]->i, argv[1]->f);
        std::cout << "setting rate slew shape: " << easing::function_info[(int)argv[1]->f] << std::endl;
    });

    addServerMethod("/set/param/cut/filter_fc_slew_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_FILTER_FC_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/filter_rq_slew_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_FILTER_RQ_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/filter_fc_rise_shape", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_FILTER_FC_RISE_SHAPE, argv[0]->i, argv[1]->f);
        std::cout << "setting fc rise shape: " << easing::function_info[(int)argv[1]->f] << std::endl;
    });

    addServerMethod("/set/param/cut/filter_fc_fall_shape", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_FILTER_FC_FALL_SHAPE, argv[0]->i, argv[1]->f);
        std::cout << "setting fc fall shape: " << easing::function_info[(int)argv[1]->f] << std::endl;
    });

    addServerMethod("/set/param/cut/filter_rq_rise_shape", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_FILTER_RQ_RISE_SHAPE, argv[0]->i, argv[1]->f);
        std::cout << "setting rq rise shape: " << easing::function_info[(int)argv[1]->f] << std::endl;
    });

    addServerMethod("/set/param/cut/filter_rq_fall_shape", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_FILTER_RQ_FALL_SHAPE, argv[0]->i, argv[1]->f);
        std::cout << "setting rq fall shape: " << easing::function_info[(int)argv[1]->f] << std::endl;
    });

    //---- other softcut voice parameters
    addServerMethod("/set/param/cut/buffer", "ii", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_BUFFER, argv[0]->i, argv[1]->i);
    });

    addServerMethod("/set/param/cut/read_duck", "ii", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_READ_DUCK_TARGET,argv[0]->i, argv[1]->i);
    });

    addServerMethod("/set/param/cut/write_duck", "ii", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_WRITE_DUCK_TARGET,argv[0]->i, argv[1]->i);
    });

    addServerMethod("/set/param/cut/follow", "ii", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_FOLLOW_TARGET,argv[0]->i, argv[1]->i);
    });

    //-------------------------------
    //--- softcut buffer manipulation

    addServerMethod("/softcut/buffer/read_mono", "sfffii", [](lo_arg **argv, int argc) {
        float startSrc = 0.f;
        float startDst = 0.f;
        float dur = -1.f;
        int chanSrc = 0;
        int chanDst = 0;
        if (argc < 1) {
            std::cerr << "/softcut/buffer/read_mono requires at least one argument (file path)" << std::endl;
            return;
        }
        if (argc > 1) {
            startSrc = argv[1]->f;
        }
        if (argc > 2) {
            startDst = argv[2]->f;
        }
        if (argc > 3) {
            dur = argv[3]->f;
        }
        if (argc > 4) {
            chanSrc = argv[4]->i;
        }
        if (argc > 5) {
            chanDst = argv[5]->i;
        }
        const char *str = &argv[0]->s;
        softCutClient->readBufferMono(str, startSrc, startDst, dur, chanSrc, chanDst);

    });

    addServerMethod("/softcut/buffer/read_stereo", "sfff", [](lo_arg **argv, int argc) {
        float startSrc = 0.f;
        float startDst = 0.f;
        float dur = -1.f;
        if (argc < 1) {
            std::cerr << "/softcut/buffer/read_stereo requires at least one argument (file path)" << std::endl;
            return;
        }
        if (argc > 1) {
            startSrc = argv[1]->f;
        }
        if (argc > 2) {
            startDst = argv[2]->f;
        }
        if (argc > 3) {
            dur = argv[3]->f;
        }
        const char *str = &argv[0]->s;
        softCutClient->readBufferStereo(str, startSrc, startDst, dur);
    });


    addServerMethod("/softcut/buffer/write_mono", "sffi", [](lo_arg **argv, int argc) {
        float start = 0.f;
        float dur = -1.f;
        int chan = 0;
        if (argc < 1) {
            std::cerr << "/softcut/buffer/write_mono requires at least one argument (file path)" << std::endl;
            return;
        }
        if (argc > 1) {
            start = argv[1]->f;
        }
        if (argc > 2) {
            dur = argv[2]->f;
        }
        if (argc > 3) {
            chan = argv[3]->i;
        }
        const char *str = &argv[0]->s;
        softCutClient->writeBufferMono(str, start, dur, chan);
    });

    addServerMethod("/softcut/buffer/write_stereo", "sff", [](lo_arg **argv, int argc) {
        float start = 0.f;
        float dur = -1.f;
        if (argc < 1) {
            std::cerr << "/softcut/buffer/write_stereo requires at least one argument (file path)" << std::endl;
            return;
        }
        if (argc > 1) {
            start = argv[1]->f;
        }
        if (argc > 2) {
            dur = argv[2]->f;
        }
        const char *str = &argv[0]->s;
        softCutClient->writeBufferStereo(str, start, dur);
    });


    addServerMethod("/softcut/buffer/clear", "", [](lo_arg **argv, int argc) {
        (void) argc;
        (void) argv;
        softCutClient->clearBuffer(0);
        softCutClient->clearBuffer(1);
    });


    addServerMethod("/softcut/buffer/clear_channel", "i", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        softCutClient->clearBuffer(argv[0]->i);
    });

    addServerMethod("/softcut/buffer/clear_region", "ff", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        softCutClient->clearBuffer(0, argv[0]->f, argv[1]->f);
        softCutClient->clearBuffer(1, argv[0]->f, argv[1]->f);
    });

    addServerMethod("/softcut/buffer/clear_region_channel", "iff", [](lo_arg **argv, int argc) {
        if (argc < 3) {
            return;
        }
        softCutClient->clearBuffer(argv[0]->i, argv[1]->f, argv[2]->f);
    });

    addServerMethod("/softcut/reset", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;

        softCutClient->clearBuffer(0, 0, -1);
        softCutClient->clearBuffer(1, 0, -1);
        phasePoll->stop();

        Commands::softcutCommands.post(Commands::Id::CUT_RESET_ALL_VOICES, 0, 0);
    });

    //---------------------
    //--- softcut polls

    addServerMethod("/set/param/cut/phase_quant", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        softCutClient->setPhaseQuant(argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/phase_offset", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) { return; }
        softCutClient->setPhaseOffset(argv[0]->i, argv[1]->f);
    });

    addServerMethod("/poll/start/cut/phase", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        phasePoll->start();
    });

    addServerMethod("/poll/stop/cut/phase", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        phasePoll->stop();
    });


    //------------------------
    //--- tape control

    addServerMethod("/tape/record/open", "s", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        mixerClient->openTapeRecord(&argv[0]->s);
    });

    addServerMethod("/tape/record/start", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        mixerClient->startTapeRecord();
    });

    addServerMethod("/tape/record/stop", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        mixerClient->stopTapeRecord();
    });

    addServerMethod("/tape/play/open", "s", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        mixerClient->openTapePlayback(&argv[0]->s);
    });

    addServerMethod("/tape/play/start", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        mixerClient->startTapePlayback();
    });

    addServerMethod("/tape/play/stop", "", [](lo_arg **argv, int argc) {
        (void) argv;
        (void) argc;
        mixerClient->stopTapePlayback();
    });

    addServerMethod("/set/level/tape", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_TAPE, argv[0]->f);
    });

    addServerMethod("/set/level/tape_rev", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_TAPE_AUX, argv[0]->f);
    });
}

void OscInterface::printServerMethods() {
    using std::cout;
    using std::endl;
    using std::string;
    using boost::format;
    cout << "osc methods: " << endl;
    for (unsigned int i = 0; i < numMethods; ++i) {
        cout << format(" %1% [%2%]") % methods[i].path % methods[i].format << endl;
    }
}

void OscInterface::deinit() {
    lo_address_free(matronAddress);
}

