

//
// Created by ezra on 11/4/18.
//

#include <thread>
#include <utility>

#include "effects/CompressorParams.h"
#include "effects/ReverbParams.h"

#include "BufDiskWorker.h"
#include "Commands.h"
#include "OscInterface.h"

using namespace crone;

bool OscInterface::quitFlag;

std::string OscInterface::port;
lo_server_thread OscInterface::st;
lo_address OscInterface::matronAddress;

std::array<OscInterface::OscMethod, OscInterface::MaxNumMethods> OscInterface::methods;
unsigned int OscInterface::numMethods = 0;

std::unique_ptr<Poll> OscInterface::vuPoll;
std::unique_ptr<Poll> OscInterface::phasePoll;
std::unique_ptr<Poll> OscInterface::tapePoll;
MixerClient *OscInterface::mixerClient;
SoftcutClient *OscInterface::softCutClient;

OscInterface::OscMethod::OscMethod(string p, string f, OscInterface::Handler h)
    : path(std::move(p)), format(std::move(f)), handler(h) {
}

void OscInterface::init(MixerClient *m, SoftcutClient *sc) {
    quitFlag = false;
    // FIXME: should get port configs from program args or elsewhere
    port = "9999";
    matronAddress = lo_address_new("127.0.0.1", "8888");

    st = lo_server_thread_new(port.c_str(), handleLoError);
    addServerMethods();

    mixerClient = m;
    softCutClient = sc;

    // FIXME: polls should really live somewhere else (client classes?)
    //--- VU poll
    vuPoll = std::make_unique<Poll>("vu");
    vuPoll->setCallback([](const char *path) {
        char l[4];

        l[0] = (uint8_t)(64 * mixerClient->getInputPeakPos(0));
        l[1] = (uint8_t)(64 * mixerClient->getInputPeakPos(1));
        l[2] = (uint8_t)(64 * mixerClient->getOutputPeakPos(0));
        l[3] = (uint8_t)(64 * mixerClient->getOutputPeakPos(1));

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

    //--- TODO: softcut trigger poll?

    //--- tape status poll
    tapePoll = std::make_unique<Poll>("tape");
    tapePoll->setCallback([](const char *path) {
        // send status snapshot
        auto s = mixerClient->getTapeStatus();
        lo_send(matronAddress, path, "iffifi",
                s.play_state, s.play_pos_s, s.play_len_s,
                s.rec_state, s.rec_pos_s, s.loop_enabled);

        // detect file-close edges so we notify matron only after drain/close is complete
        static bool prevRecHasFile = false;
        static bool prevPlayHasFile = false;

        bool playHasFile = (s.play_state != 0);
        bool recHasFile = (s.rec_state != 0);

        if (prevRecHasFile && !recHasFile) {
            lo_send(matronAddress, "/tape/record/close", "");
        }
        if (prevPlayHasFile && !playHasFile) {
            lo_send(matronAddress, "/tape/play/close", "");
        }

        prevRecHasFile = recHasFile;
        prevPlayHasFile = playHasFile;
    });
    tapePoll->setPeriod(50);

    lo_server_thread_start(st);
}

void OscInterface::addServerMethod(const char *path, const char *format, Handler handler) {
    OscMethod m(path, format, handler);
    methods[numMethods] = m;
    lo_server_thread_add_method(st, path, format, [](const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *data) -> int {
                                    (void) path;
                                    (void) types;
                                    (void) msg;
                                    auto pm = static_cast<OscMethod *>(data);
                                    //std::cerr << "osc rx: " << path << std::endl;
                                    pm->handler(argv, argc);
                                    return 0; }, &(methods[numMethods]));
    numMethods++;
}

void OscInterface::addServerMethods() {
    addServerMethod("/hello", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        std::cout << "hello" << std::endl;
    });

    addServerMethod("/goodbye", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        std::cout << "goodbye" << std::endl;
        OscInterface::quitFlag = true;
    });

    addServerMethod("/quit", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        OscInterface::quitFlag = true;
    });

    //---------------------------
    //--- mixer polls

    addServerMethod("/poll/start/vu", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        vuPoll->start();
    });

    addServerMethod("/poll/stop/vu", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        vuPoll->stop();
    });

    //--- tape poll control
    addServerMethod("/poll/start/tape", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        tapePoll->start();
    });
    addServerMethod("/poll/stop/tape", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        tapePoll->stop();
    });

    ////////////////////////////////
    /// FIXME: many of these methods are trivial setters;
    // they could simply be structured around atomic fields instead of requiring the Command queue.
    // would require some refactoring e.g. to expose level ramp targets as atomics

    //--------------------------
    //--- levels
    addServerMethod("/set/level/adc", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_ADC, argv[0]->f);
    });

    addServerMethod("/set/level/dac", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_DAC, argv[0]->f);
    });

    addServerMethod("/set/level/ext", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_EXT, argv[0]->f);
    });

    addServerMethod("/set/level/cut_master", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_CUT_MASTER, argv[0]->f);
    });

    addServerMethod("/set/level/ext_rev", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_EXT_AUX, argv[0]->f);
    });

    addServerMethod("/set/level/rev_dac", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_AUX_DAC, argv[0]->f);
    });

    addServerMethod("/set/level/monitor", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_MONITOR, argv[0]->f);
    });

    addServerMethod("/set/level/monitor_mix", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_MONITOR_MIX, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/level/monitor_rev", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_MONITOR_AUX, argv[0]->f);
    });

    addServerMethod("/set/level/compressor_mix", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_INS_MIX, argv[0]->f);
    });

    // toggle enabled
    addServerMethod("/set/enabled/compressor", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_ENABLED_COMPRESSOR, argv[0]->f);
    });

    addServerMethod("/set/enabled/reverb", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_ENABLED_REVERB, argv[0]->f);
    });

    //-------------------------
    //-- compressor params

    addServerMethod("/set/param/compressor/ratio", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::RATIO, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/threshold", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::THRESHOLD, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/attack", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::ATTACK, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/release", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::RELEASE, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/gain_pre", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::GAIN_PRE, argv[0]->f);
    });

    addServerMethod("/set/param/compressor/gain_post", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_COMPRESSOR, CompressorParam::GAIN_POST, argv[0]->f);
    });

    //--------------------------
    //-- reverb params

    addServerMethod("/set/param/reverb/pre_del", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_REVERB, ReverbParam::PRE_DEL, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/lf_fc", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_REVERB, ReverbParam::LF_FC, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/low_rt60", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_REVERB, ReverbParam::LOW_RT60, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/mid_rt60", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_REVERB, ReverbParam::MID_RT60, argv[0]->f);
    });

    addServerMethod("/set/param/reverb/hf_damp", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_PARAM_REVERB, ReverbParam::HF_DAMP, argv[0]->f);
    });

    //--------------------------------
    //-- softcut routing

    addServerMethod("/set/enabled/cut", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_ENABLED_CUT, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/level/cut", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_LEVEL_CUT, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/pan/cut", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_PAN_CUT, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/level/adc_cut", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_ADC_CUT, argv[0]->f);
    });

    addServerMethod("/set/level/ext_cut", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_EXT_CUT, argv[0]->f);
    });

    addServerMethod("/set/level/tape_cut", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_TAPE_CUT, argv[0]->f);
    });

    addServerMethod("/set/level/cut_rev", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_CUT_AUX, argv[0]->f);
    });

    //--- NB: these are handled by the softcut command queue,
    // because their corresponding mix points are processed by the softcut client.

    // input channel -> voice levels
    addServerMethod("/set/level/in_cut", "iif", [](lo_arg **argv, int argc) {
        if (argc < 3) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_LEVEL_IN_CUT, argv[0]->i, argv[1]->i, argv[2]->f);
    });

    // voice ->  voice levels
    addServerMethod("/set/level/cut_cut", "iif", [](lo_arg **argv, int argc) {
        if (argc < 3) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_LEVEL_CUT_CUT, argv[0]->i, argv[1]->i, argv[2]->f);
    });

    //--------------------------------
    //-- softcut params

    addServerMethod("/set/param/cut/rate", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_RATE, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/loop_start", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_LOOP_START, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/loop_end", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_LOOP_END, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/loop_flag", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_LOOP_FLAG, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/fade_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_FADE_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/rec_level", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_REC_LEVEL, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_level", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PRE_LEVEL, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/rec_flag", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_REC_FLAG, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/play_flag", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PLAY_FLAG, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/rec_offset", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_REC_OFFSET, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/position", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_POSITION, argv[0]->i, argv[1]->f);
    });

    // --- input filter
    addServerMethod("/set/param/cut/pre_filter_fc", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PRE_FILTER_FC, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_filter_fc_mod", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PRE_FILTER_FC_MOD, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_filter_rq", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PRE_FILTER_RQ, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_filter_lp", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PRE_FILTER_LP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_filter_hp", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PRE_FILTER_HP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_filter_bp", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PRE_FILTER_BP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_filter_br", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PRE_FILTER_BR, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pre_filter_dry", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PRE_FILTER_DRY, argv[0]->i, argv[1]->f);
    });

    // --- output filter
    addServerMethod("/set/param/cut/post_filter_fc", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_POST_FILTER_FC, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_rq", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_POST_FILTER_RQ, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_lp", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_POST_FILTER_LP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_hp", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_POST_FILTER_HP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_bp", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_POST_FILTER_BP, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_br", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_POST_FILTER_BR, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/post_filter_dry", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_POST_FILTER_DRY, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/voice_sync", "iif", [](lo_arg **argv, int argc) {
        if (argc < 3) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_VOICE_SYNC, argv[0]->i, argv[1]->i, argv[2]->f);
    });

    ///////////////////////////////////////////
    /// FIXME: fade curve calculations are now per-voice,
    /// so these methods won't work at all without modification.
    //// in any case, they should use worker threads and this is the wrong place to manage that.

#if 0
    addServerMethod("/set/param/cut/pre_fade_window", "if", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        float x = argv[0]->f;
        auto t = std::thread([x] {
            FadeCurves::setPreWindowRatio(x);
        });
        t.detach();
    });

    addServerMethod("/set/param/cut/rec_fade_delay", "if", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        float x = argv[0]->f;
        auto t = std::thread([x] {
            FadeCurves::setRecDelayRatio(x);
        });
        t.detach();
    });

    addServerMethod("/set/param/cut/pre_fade_shape", "if", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        float x = argv[0]->f;
        auto t = std::thread([x] {
            FadeCurves::setPreShape(static_cast<FadeCurves::Shape>(x));
        });
        t.detach();
    });

    addServerMethod("/set/param/cut/rec_fade_shape", "if", [](lo_arg **argv, int argc) {
        if (argc < 1) { return; }
        float x = argv[0]->f;
        auto t = std::thread([x] {
            FadeCurves::setRecShape(static_cast<FadeCurves::Shape>(x));
        });
        t.detach();
    });
#endif
    //////////////////

    addServerMethod("/set/param/cut/level_slew_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_LEVEL_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/pan_slew_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_PAN_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/recpre_slew_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_RECPRE_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/rate_slew_time", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_RATE_SLEW_TIME, argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/buffer", "ii", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        Commands::softcutCommands.post(Commands::Id::SET_CUT_BUFFER, argv[0]->i, argv[1]->i);
    });

    //-------------------------------
    //--- softcut buffer manipulation

    // FIXME: hrm, our system doesn't allow variable argument count. maybe need to make multiple methods
    addServerMethod("/softcut/buffer/read_mono", "sfffiiff", [](lo_arg **argv, int argc) {
        float startSrc = 0.f;
        float startDst = 0.f;
        float dur = -1.f;
        int chanSrc = 0;
        int chanDst = 0;
        float preserve = 0.f;
        float mix = 1.f;
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
        if (argc > 6) {
            preserve = argv[6]->f;
        }
        if (argc > 7) {
            mix = argv[7]->f;
        }
        const char *str = &argv[0]->s;
        softCutClient->readBufferMono(str, startSrc, startDst, dur, chanSrc, chanDst, preserve, mix);
    });

    // FIXME: hrm, our system doesn't allow variable argument count. maybe need to make multiple methods
    addServerMethod("/softcut/buffer/read_stereo", "sfffff", [](lo_arg **argv, int argc) {
        float startSrc = 0.f;
        float startDst = 0.f;
        float dur = -1.f;
        float preserve = 0.f;
        float mix = 1.f;
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
        if (argc > 4) {
            preserve = argv[4]->f;
        }
        if (argc > 5) {
            mix = argv[5]->f;
        }
        const char *str = &argv[0]->s;
        softCutClient->readBufferStereo(str, startSrc, startDst, dur, preserve, mix);
    });

    // FIXME: hrm, our system doesn't allow variable argument count. maybe need to make multiple methods
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

    // FIXME: hrm, our system doesn't allow variable argument count. maybe need to make multiple methods
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
        (void)argc;
        (void)argv;
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

    addServerMethod("/softcut/buffer/clear_fade_region", "ffff", [](lo_arg **argv, int argc) {
        float dur = -1;
        float fadeTime = 0;
        float preserve = 0;
        if (argc < 1) {
            return;
        }
        if (argc > 1) {
            dur = argv[1]->f;
        }
        if (argc > 2) {
            fadeTime = argv[2]->f;
        }
        if (argc > 3) {
            preserve = argv[3]->f;
        }
        softCutClient->clearBufferWithFade(0, argv[0]->f, dur, fadeTime, preserve);
        softCutClient->clearBufferWithFade(1, argv[0]->f, dur, fadeTime, preserve);
    });

    addServerMethod("/softcut/buffer/clear_fade_region_channel", "iffff", [](lo_arg **argv, int argc) {
        float dur = -1;
        float fadeTime = 0;
        float preserve = 0;
        if (argc < 2) {
            return;
        }
        if (argc > 2) {
            dur = argv[2]->f;
        }
        if (argc > 3) {
            fadeTime = argv[3]->f;
        }
        if (argc > 4) {
            preserve = argv[4]->f;
        }
        softCutClient->clearBufferWithFade(argv[0]->i, argv[1]->f, dur, fadeTime, preserve);
    });

    addServerMethod("/softcut/buffer/copy_mono", "iifffffi", [](lo_arg **argv, int argc) {
        float dur = -1.f;
        float fadeTime = 0.f;
        float preserve = 0.f;
        bool reverse = false;
        if (argc < 4) {
            return;
        }
        if (argc > 4) {
            dur = argv[4]->f;
        }
        if (argc > 5) {
            fadeTime = argv[5]->f;
        }
        if (argc > 6) {
            preserve = argv[6]->f;
        }
        if (argc > 7) {
            reverse = argv[7]->i != 0;
        }

        softCutClient->copyBuffer(argv[0]->i, argv[1]->i, argv[2]->f, argv[3]->f, dur, fadeTime, preserve, reverse);
    });

    addServerMethod("/softcut/buffer/copy_stereo", "fffffi", [](lo_arg **argv, int argc) {
        float dur = -1.f;
        float fadeTime = 0.f;
        float preserve = 0.f;
        bool reverse = false;
        if (argc < 2) {
            return;
        }
        if (argc > 2) {
            dur = argv[2]->f;
        }
        if (argc > 3) {
            fadeTime = argv[3]->f;
        }
        if (argc > 4) {
            preserve = argv[4]->f;
        }
        if (argc > 5) {
            reverse = argv[5]->i != 0;
        }

        softCutClient->copyBuffer(0, 0, argv[0]->f, argv[1]->f, dur, fadeTime, preserve, reverse);
        softCutClient->copyBuffer(1, 1, argv[0]->f, argv[1]->f, dur, fadeTime, preserve, reverse);
    });

    addServerMethod("/softcut/buffer/render", "iffi", [](lo_arg **argv, int argc) {
        int sampleCt = 128;
        if (argc < 3) {
            return;
        }

        int ch = argv[0]->i;
        if (argc > 3) {
            sampleCt = argv[3]->i;
        }

        softCutClient->renderSamples(ch, argv[1]->f, argv[2]->f, sampleCt,
                                     [=](float secPerSample, float start, size_t count, float *samples) {
                                         lo_blob bl = lo_blob_new(count * sizeof(float), samples);
                                         lo_send(matronAddress, "/softcut/buffer/render_callback", "iffb", ch, secPerSample, start, bl);
                                     });
    });

    addServerMethod("/softcut/query/position", "i", [](lo_arg **argv, int argc) {
        if (argc < 1)
            return;
        int idx = argv[0]->i;
        float pos = softCutClient->getPosition(idx);
        lo_send(matronAddress, "/poll/softcut/position", "if", idx, pos);
    });

    addServerMethod("/softcut/reset", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;

        softCutClient->clearBuffer(0, 0, -1);
        softCutClient->clearBuffer(1, 0, -1);

        softCutClient->reset();
        for (int i = 0; i < SoftcutClient::NumVoices; ++i) {
            phasePoll->stop();
        }
    });

    //---------------------
    //--- softcut polls

    addServerMethod("/set/param/cut/phase_quant", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        softCutClient->setPhaseQuant(argv[0]->i, argv[1]->f);
    });

    addServerMethod("/set/param/cut/phase_offset", "if", [](lo_arg **argv, int argc) {
        if (argc < 2) {
            return;
        }
        softCutClient->setPhaseOffset(argv[0]->i, argv[1]->f);
    });

    addServerMethod("/poll/start/cut/phase", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        phasePoll->start();
    });

    addServerMethod("/poll/stop/cut/phase", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        phasePoll->stop();
    });

    //------------------------
    //--- tape control

    addServerMethod("/tape/record/open", "s", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        const char *p = &argv[0]->s;
        mixerClient->openTapeRecord(p);
        // notify matron of the armed record file
        lo_send(matronAddress, "/tape/record/open", "s", p);
    });

    addServerMethod("/tape/record/start", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        mixerClient->startTapeRecord();
    });

    addServerMethod("/tape/record/pause", "i", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        mixerClient->pauseTapeRecord(argv[0]->i != 0);
    });

    addServerMethod("/tape/record/stop", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        mixerClient->stopTapeRecord();
    });

    addServerMethod("/tape/play/open", "s", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        const char *p = &argv[0]->s;
        mixerClient->openTapePlayback(p);
        // notify matron of the opened playback file
        lo_send(matronAddress, "/tape/play/open", "s", p);
    });

    addServerMethod("/tape/play/start", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        mixerClient->startTapePlayback();
    });

    addServerMethod("/tape/play/pause", "i", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        mixerClient->pauseTapePlayback(argv[0]->i != 0);
    });

    addServerMethod("/tape/play/stop", "", [](lo_arg **argv, int argc) {
        (void)argv;
        (void)argc;
        mixerClient->stopTapePlayback();
    });

    addServerMethod("/tape/play/loop", "i", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        mixerClient->setTapeLoop(argv[0]->i != 0);
    });

    addServerMethod("/set/level/tape", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_TAPE, argv[0]->f);
    });

    addServerMethod("/set/level/tape_rev", "f", [](lo_arg **argv, int argc) {
        if (argc < 1) {
            return;
        }
        Commands::mixerCommands.post(Commands::Id::SET_LEVEL_TAPE_AUX, argv[0]->f);
    });
}

void OscInterface::printServerMethods() {
    std::cout << "osc methods: " << std::endl;
    for (unsigned int i = 0; i < numMethods; ++i) {
        std::cout << methods[i].path << " [" << methods[i].format << "]" << std::endl;
    }
}

void OscInterface::deinit() {
    lo_address_free(matronAddress);
}
