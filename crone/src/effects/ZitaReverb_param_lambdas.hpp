addServerMethod("/set/param/ZitaReverb/pre_del", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_ZITAREVERB_PRE_DEL, argv[0]->f);
});

addServerMethod("/set/param/ZitaReverb/lf_fc", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_ZITAREVERB_LF_FC, argv[0]->f);
});

addServerMethod("/set/param/ZitaReverb/low_rt60", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_ZITAREVERB_LOW_RT60, argv[0]->f);
});

addServerMethod("/set/param/ZitaReverb/mid_rt60", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_ZITAREVERB_MID_RT60, argv[0]->f);
});

addServerMethod("/set/param/ZitaReverb/hf_damp", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_ZITAREVERB_HF_DAMP, argv[0]->f);
});

