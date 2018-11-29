//
// Created by emb on 11/28/18.
//

#include "SoftCutClient.h"

crone::SoftCutClient::SoftCutClient() : Client<4, 2>("softcut") {}

void crone::SoftCutClient::process(jack_nframes_t numFrames) {
    mixInput(numFrames);
    // process softcuts (overwrites output bus)
    for(int v=0; v<NUM_VOICES; ++v) {
        if (!enabled[v]) {
            continue;
        }
        cut.processBlock(v, input[v].buf[0], output[v].buf[0], static_cast<int>(numFrames));
    }
    mixOutput(numFrames);
    mix.copyTo(sink[0], numFrames);
}

void crone::SoftCutClient::setSampleRate(jack_nframes_t sr) {
    cut.setSampleRate(sr);
}

void crone::SoftCutClient::mixInput(size_t numFrames) {
    for(int ch=0; ch<2; ++ch) {
        for(int v=0; v<NUM_VOICES; ++v) {
            input[v].mixFrom(&source[SOURCE_ADC][ch], numFrames, in_adc[ch][v]);
            input[v].mixFrom(&source[SOURCE_EXT][ch], numFrames, in_ext[ch][v]);
            for(int w=0; w<NUM_VOICES; ++w) {
                input[v].mixFrom(output[w], numFrames, feedback[v][w]);
            }
        }
    }
}

void crone::SoftCutClient::mixOutput(size_t numFrames) {
    for(int v=0; v<NUM_VOICES; ++v) {
        mix.panMixFrom(output[v], numFrames, out_level[v], out_pan[v]);
    }
}
