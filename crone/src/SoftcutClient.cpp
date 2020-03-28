//
// Created by emb on 11/28/18.
//

#include <sndfile.hh>

#include "BufDiskWorker.h"
#include "Commands.h"
#include "SoftcutClient.h"

using namespace crone;

// clamp unsigned int to upper bound, inclusive
static inline void clamp(size_t &x, const size_t a) {
    if (x > a) { x = a; }
}

SoftcutClient::SoftcutClient() : JackClient<2, 2>("softcut") {
    for (unsigned int i = 0; i < NumVoices; ++i) {
        cut.voice(i)->setBuffer(buf[i & 1], BufFrames);
    }
    bufIdx[0] = BufDiskWorker::registerBuffer(buf[0], BufFrames);
    bufIdx[1] = BufDiskWorker::registerBuffer(buf[1], BufFrames);
}

void SoftcutClient::process(jack_nframes_t numFrames) {
    Commands::softcutCommands.handlePending(this);
    clearBusses(numFrames);
    mixInput(numFrames);
    // process softcuts (overwrites output bus)
    for (int v = 0; v < NumVoices; ++v) {
        if (enabled[v]) {
            cut.processBlock(v, input[v].buf[0], output[v].buf[0], static_cast<int>(numFrames));
        }
    }
    mixOutput(numFrames);
    mix.copyTo(sink[0], numFrames);
}

void SoftcutClient::setSampleRate(jack_nframes_t sr) {
    cut.setSampleRate(sr);
}

void SoftcutClient::clearBusses(size_t numFrames) {
    mix.clear(numFrames);
    for (auto &b : input) { b.clear(numFrames); }
}

void SoftcutClient::mixInput(size_t numFrames) {
    for (int dst = 0; dst < NumVoices; ++dst) {
        if (cut.voice(dst)->getRecFlag()) {
            for (int ch = 0; ch < 2; ++ch) {
                input[dst].mixFrom(&source[SourceAdc][ch], numFrames, inLevel[ch][dst]);
            }
            for (int src = 0; src < NumVoices; ++src) {
                if (cut.voice(src)->getPlayFlag()) {
                    input[dst].mixFrom(output[src], numFrames, fbLevel[src][dst]);
                }
            }
        }
    }
}

void SoftcutClient::mixOutput(size_t numFrames) {
    for (int v = 0; v < NumVoices; ++v) {
        if (cut.voice(v)->getPlayFlag()) {
            mix.panMixEpFrom(output[v], numFrames, outLevel[v], outPan[v]);
        }
    }
}

void SoftcutClient::handleCommand(Commands::CommandPacket *p) {
    switch (p->id) {
        //-- client routing and levels
        case Commands::Id::SET_ENABLED_CUT:
            enabled[p->idx_0] = p->value > 0.f;
            break;
        case Commands::Id::SET_LEVEL_IN_CUT:
            inLevel[p->idx_0][p->idx_1].setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_CUT_CUT:
            fbLevel[p->idx_0][p->idx_1].setTarget(p->value);
            break;
            //-- voice levels, pan
        case Commands::Id::SET_PAN_CUT:
            outPan[p->idx_0].setTarget((p->value / 2) + 0.5); // map -1,1 to 0,1
            break;
        case Commands::Id::SET_LEVEL_CUT:
            outLevel[p->idx_0].setTarget(p->value);
            break;
            //-- voice commands
        case Commands::Id::SET_CUT_VOICE_BUFFER:
            cut.voice(p->idx_0)->setBuffer(buf[p->idx_1], BufFrames);
            break;
        case Commands::Id::SET_CUT_VOICE_REC_ENABLED:
            cut.voice(p->idx_0)->setRecFlag(p->value > 0.f);
            break;
        case Commands::Id::SET_CUT_VOICE_PLAY_ENABLED:
            cut.voice(p->idx_0)->setPlayFlag(p->value > 0.f);
            break;
        case Commands::Id::SET_CUT_VOICE_RATE:
            cut.voice(p->idx_0)->setRate(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_LOOP_START:
            cut.voice(p->idx_0)->setLoopStart(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_LOOP_END:
            cut.voice(p->idx_0)->setLoopEnd(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_LOOP_ENABLED:
            cut.voice(p->idx_0)->setLoopFlag(p->value > 0.f);
            break;
        case Commands::Id::SET_CUT_VOICE_FADE_TIME:
            cut.voice(p->idx_0)->setFadeTime(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_REC_LEVEL:
            cut.voice(p->idx_0)->setRecLevel(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_PRE_LEVEL:
            cut.voice(p->idx_0)->setPreLevel(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_REC_OFFSET:
            cut.voice(p->idx_0)->setRecOffset(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_POSITION:
            cut.voice(p->idx_0)->setPosition(p->value);
            break;

            //-- input filter
        case Commands::Id::SET_CUT_VOICE_PRE_FILTER_FC:
            cut.voice(p->idx_0)->setPreFilterFc(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_PRE_FILTER_FC_MOD:
            cut.voice(p->idx_0)->setPreFilterFcMod(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_PRE_FILTER_Q:
            cut.voice(p->idx_0)->setPreFilterQ(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_PRE_FILTER_ENABLED:
            cut.voice(p->idx_0)->setPreFilterEnabled(p->value > 0);
            break;

            //-- output filter
        case Commands::Id::SET_CUT_VOICE_POST_FILTER_FC:
            cut.voice(p->idx_0)->setPostFilterFc(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_POST_FILTER_RQ:
            cut.voice(p->idx_0)->setPostFilterRq(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_POST_FILTER_LP:
            cut.voice(p->idx_0)->setPostFilterLp(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_POST_FILTER_HP:
            cut.voice(p->idx_0)->setPostFilterHp(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_POST_FILTER_BP:
            cut.voice(p->idx_0)->setPostFilterBp(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_POST_FILTER_BR:
            cut.voice(p->idx_0)->setPostFilterBr(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_POST_FILTER_DRY:
            cut.voice(p->idx_0)->setPostFilterDry(p->value);
            break;

            //-- slew times
        case Commands::Id::SET_CUT_VOICE_LEVEL_SLEW_TIME:
            outLevel[p->idx_0].setTime(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_PAN_SLEW_TIME:
            outPan[p->idx_0].setTime(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_RECPRE_SLEW_TIME:
            cut.voice(p->idx_0)->setRecPreSlewTime(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_RATE_SLEW_TIME:
            cut.voice(p->idx_0)->setRateSlewTime(p->value);
            break;

            //-- sync / inter-voice
        case Commands::Id::SET_CUT_VOICE_SYNC:
            cut.syncVoice(p->idx_0, p->idx_1, p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_DUCK_TARGET:
            cut.voice(p->idx_0)->setDuckTarget(cut.voice(p->idx_1));
        default:;;
    }
}

void SoftcutClient::reset() {

    for (int v = 0; v < NumVoices; ++v) {
        cut.voice(v)->setBuffer(buf[v % 2], BufFrames);
        outLevel[v].setTarget(0.f);
        outLevel->setTime(0.001);
        outPan[v].setTarget(0.5f);
        outPan->setTime(0.001);

        enabled[v] = false;

        setPhaseQuant(v, 1.f);
        setPhaseOffset(v, 0.f);

        for (int i = 0; i < 2; ++i) {
            inLevel[i][v].setTime(0.001);
            inLevel[i][v].setTarget(0.0);
        }

        for (int w = 0; w < NumVoices; ++w) {
            fbLevel[v][w].setTime(0.001);
            fbLevel[v][w].setTarget(0.0);
        }

        cut.voice(v)->setLoopStart(v * 2);
        cut.voice(v)->setLoopEnd(v * 2 + 1);

        output[v].clear();
        input[v].clear();
    }
    cut.reset();
}
