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

