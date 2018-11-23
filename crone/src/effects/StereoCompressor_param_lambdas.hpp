addServerMethod("/set/param/StereoCompressor/ratio", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_STEREOCOMPRESSOR_RATIO, argv[0]->f);
});

addServerMethod("/set/param/StereoCompressor/threshold", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_STEREOCOMPRESSOR_THRESHOLD, argv[0]->f);
});

addServerMethod("/set/param/StereoCompressor/attack", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_STEREOCOMPRESSOR_ATTACK, argv[0]->f);
});

addServerMethod("/set/param/StereoCompressor/release", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_STEREOCOMPRESSOR_RELEASE, argv[0]->f);
});

addServerMethod("/set/param/StereoCompressor/gain_pre", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_STEREOCOMPRESSOR_GAIN_PRE, argv[0]->f);
});

addServerMethod("/set/param/StereoCompressor/gain_post", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_STEREOCOMPRESSOR_GAIN_POST, argv[0]->f);
});

