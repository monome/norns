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

