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

SoftcutClient::SoftcutClient() : Client<2, 2>("softcut") {
    for (unsigned int i = 0; i < NumVoices; ++i) {
        cut.voice(i)->setBuffer(buf[i & 1], BufFrames);
        cut.setInputBus((int)i, inputBus[i].buf[0]);
        cut.setOutputBus((int)i, outputBus[i].buf[0]);

    }
    bufIdx[0] = BufDiskWorker::registerBuffer(buf[0], BufFrames);
    bufIdx[1] = BufDiskWorker::registerBuffer(buf[1], BufFrames);
}

void SoftcutClient::process(jack_nframes_t numFrames) {
    Commands::softcutCommands.handlePending(this);
    clearBusses(numFrames);
    mixInput(numFrames);
    cut.processBlock(numFrames); // overwrites output bus
    mixOutput(numFrames);
    mix.copyTo(sink[0], numFrames);
}

void SoftcutClient::setSampleRate(jack_nframes_t sr) {
    cut.setSampleRate(sr);
    for (int i=0; i<NumVoices; ++i) {
        inLevel[0][i].setSampleRate(sr);
        inLevel[1][i].setSampleRate(sr);
        outLevel[i].setSampleRate(sr);
        outPan[i].setSampleRate(sr);
        for (int j=0; j<NumVoices; ++j) {
            fbLevel[j][i].setSampleRate(sr);
        }
    }
}

void SoftcutClient::clearBusses(size_t numFrames) {
    mix.clear(numFrames);
    for (auto &b : inputBus) { b.clear(numFrames); }
}

void SoftcutClient::mixInput(size_t numFrames) {
    for (int dst = 0; dst < NumVoices; ++dst) {
        if (cut.voice(dst)->getRecFlag()) {
            // mix from capture bus to voice input bus
            for (int ch = 0; ch < 2; ++ch) {
                inputBus[dst].mixFrom(&source[SourceAdc][ch], numFrames, inLevel[ch][dst]);
            }
            // mix from feedback matrix to voice input bus
            for (int src = 0; src < NumVoices; ++src) {
                if (cut.voice(src)->getPlayFlag()) {
                    inputBus[dst].mixFrom(outputBus[src], numFrames, fbLevel[src][dst]);
                }
            }
        }
    }
}

void SoftcutClient::mixOutput(size_t numFrames) {
    for (int v = 0; v < NumVoices; ++v) {
        if (cut.getVoiceEnabled(v)) {
            if (cut.voice(v)->getPlayFlag()) {
                mix.panMixEpFrom(outputBus[v], numFrames, outLevel[v], outPan[v]);
            }
        }
    }
}

void SoftcutClient::handleCommand(Commands::CommandPacket *p) {
    softcut::Voice *v;
    switch (p->id) {
        //-- client routing and levels
        case Commands::Id::SET_ENABLED_CUT:
            //enabled[p->idx_0] = p->value > 0.f;
            cut.setVoiceEnabled(p->idx_0, p->value > 0.f);
            break;
        case Commands::Id::SET_LEVEL_IN_CUT:
            inLevel[p->idx_0][p->idx_1].setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_CUT_CUT:
            fbLevel[p->idx_0][p->idx_1].setTarget(p->value);
            break;

            //-- voice levels, pan
        case Commands::Id::SET_CUT_VOICE_PAN:
            outPan[p->idx_0].setTarget((p->value * 0.5f) + 0.5f); // map -1,1 to 0,1
            break;
        case Commands::Id::SET_CUT_VOICE_LEVEL:
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
        case Commands::Id::SET_CUT_VOICE_POST_FILTER_ENABLED:
            cut.voice(p->idx_0)->setPostFilterEnabled(p->value > 0);
            break;

            //-- slew times, shaoes
        case Commands::Id::SET_CUT_VOICE_LEVEL_SLEW_TIME:
            outLevel[p->idx_0].setTime(p->value);
            cut.voice(p->idx_0)->setLevelSlewTime(p->value);
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
        case Commands::Id::SET_CUT_VOICE_RATE_SLEW_SHAPE:
            cut.voice(p->idx_0)->setRateSlewShape(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_FILTER_FC_SLEW_TIME:
            cut.voice(p->idx_0)->setPostFilterFcSlewTime(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_FILTER_RQ_SLEW_TIME:
            cut.voice(p->idx_0)->setPostFilterRqSlewTime(p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_FILTER_FC_RISE_SHAPE:
            cut.voice(p->idx_0)->setPostFilterFcRiseShape(static_cast<int>(p->value));
            break;
        case Commands::Id::SET_CUT_VOICE_FILTER_FC_FALL_SHAPE:
            cut.voice(p->idx_0)->setPostFilterFcFallShape(static_cast<int>(p->value));
            break;
        case Commands::Id::SET_CUT_VOICE_FILTER_RQ_RISE_SHAPE:
            cut.voice(p->idx_0)->setPostFilterRqRiseShape(static_cast<int>(p->value));
            break;
        case Commands::Id::SET_CUT_VOICE_FILTER_RQ_FALL_SHAPE:
            cut.voice(p->idx_0)->setPostFilterRqFallShape(static_cast<int>(p->value));
            break;

            //-- sync / inter-voice
        case Commands::Id::SET_CUT_VOICE_SYNC:
            cut.syncVoice(p->idx_0, p->idx_1, p->value);
            break;
        case Commands::Id::SET_CUT_VOICE_READ_DUCK_TARGET:
            v = (p->idx_1 < 0) ? nullptr : cut.voice(p->idx_1);
            std::cout << "setting read duck target; voice " << p->idx_0 << "; target voice: " << v << std::endl;
            cut.voice(p->idx_0)->setReadDuckTarget(v);
            break;
        case Commands::Id::SET_CUT_VOICE_WRITE_DUCK_TARGET:
            v = (p->idx_1) < 0 ? nullptr : cut.voice(p->idx_1);
            std::cout << "setting write duck target; voice " << p->idx_0 << "; target voice: " << v << std::endl;
            cut.voice(p->idx_0)->setWriteDuckTarget(v);
            break;
        case Commands::Id::SET_CUT_VOICE_FOLLOW_TARGET:
            v = (p->idx_1) < 0 ? nullptr : cut.voice(p->idx_1);
            std::cout << "setting follow target; voice " << p->idx_0 << "; target voice: " << v << std::endl;
            cut.voice(p->idx_0)->setFollowTarget(v);
            break;
        case Commands::Id::CUT_RESET_ALL_VOICES:
            this->reset();
            break;
        default:;;
    }
}

void SoftcutClient::reset() {

    for (int v = 0; v < NumVoices; ++v) {
        cut.voice(v)->setBuffer(buf[v % 2], BufFrames);
        outLevel->setTime(0.001);
        outLevel[v].setTarget(0.f);
        outPan->setTime(0.001);
        outPan[v].setTarget(0.5f);

        enabled[v] = false;

        setPhaseQuant(v, 1.f);
        setPhaseOffset(v, 0.f);

        for (int ch = 0; ch < 2; ++ch) {
            inLevel[ch][v].setTime(0.001);
            inLevel[ch][v].setTarget(0.0);
        }

        for (int w = 0; w < NumVoices; ++w) {
            fbLevel[v][w].setTime(0.001);
            fbLevel[v][w].setTarget(0.0);
        }

        cut.voice(v)->setLoopStart((float)v * 2);
        cut.voice(v)->setLoopEnd((float)v * 2 + 1);

        outputBus[v].clear();
        inputBus[v].clear();
    }
    cut.reset();
}
