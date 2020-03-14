//
// Created by emb on 11/28/18.
//

#include <sndfile.hh>

#include "BufDiskWorker.h"
#include "Commands.h"
#include "SoftcutClient.h"


// clamp unsigned int to upper bound, inclusive
static inline void clamp(size_t &x, const size_t a) {
    if (x > a) { x = a; }
}

crone::SoftcutClient::SoftcutClient() : Client<2, 2>("softcut") {
    for (unsigned int i = 0; i < NumVoices; ++i) {
        cut.setVoiceBuffer(i, buf[i & 1], BufFrames);
    }
    bufIdx[0] = BufDiskWorker::registerBuffer(buf[0], BufFrames);
    bufIdx[1] = BufDiskWorker::registerBuffer(buf[1], BufFrames);

}

void crone::SoftcutClient::process(jack_nframes_t numFrames) {
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

void crone::SoftcutClient::setSampleRate(jack_nframes_t sr) {
    cut.setSampleRate(sr);
}


void crone::SoftcutClient::clearBusses(size_t numFrames) {
    mix.clear(numFrames);
    for (auto &b : input) { b.clear(numFrames); }
}

void crone::SoftcutClient::mixInput(size_t numFrames) {
    for (int dst = 0; dst < NumVoices; ++dst) {
        if (cut.getRecFlag(dst)) {
            for (int ch = 0; ch < 2; ++ch) {
                input[dst].mixFrom(&source[SourceAdc][ch], numFrames, inLevel[ch][dst]);
            }
            for (int src = 0; src < NumVoices; ++src) {
                if (cut.getPlayFlag(src)) {
                    input[dst].mixFrom(output[src], numFrames, fbLevel[src][dst]);
                }
            }
        }
    }
}

void crone::SoftcutClient::mixOutput(size_t numFrames) {
    for (int v = 0; v < NumVoices; ++v) {
        if (cut.getPlayFlag(v)) {
            mix.panMixEpFrom(output[v], numFrames, outLevel[v], outPan[v]);
        }
    }
}

void crone::SoftcutClient::handleCommand(Commands::CommandPacket *p) {
    switch (p->id) {
        //-- softcut routing
    case Commands::Id::SET_ENABLED_CUT:
	enabled[p->idx_0] = p->value > 0.f;
	break;
    case Commands::Id::SET_LEVEL_CUT:
	outLevel[p->idx_0].setTarget(p->value);
	break;;
    case Commands::Id::SET_PAN_CUT:
	outPan[p->idx_0].setTarget((p->value/2)+0.5); // map -1,1 to 0,1
	break;
    case Commands::Id::SET_LEVEL_IN_CUT:
	inLevel[p->idx_0][p->idx_1].setTarget(p->value);
	break;
    case Commands::Id::SET_LEVEL_CUT_CUT:
	fbLevel[p->idx_0][p->idx_1].setTarget(p->value);
	break;
	//-- softcut commands
    case Commands::Id::SET_CUT_RATE:
	cut.setRate(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_LOOP_START:
	cut.setLoopStart(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_LOOP_END:
	cut.setLoopEnd(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_LOOP_FLAG:
	cut.setLoopFlag(p->idx_0, p->value > 0.f);
	break;
    case Commands::Id::SET_CUT_FADE_TIME:
	cut.setFadeTime(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_REC_LEVEL:
	cut.setRecLevel(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_PRE_LEVEL:
	cut.setPreLevel(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_REC_FLAG:
	cut.setRecFlag(p->idx_0, p->value > 0.f);
	break;
    case Commands::Id::SET_CUT_PLAY_FLAG:
	cut.setPlayFlag(p->idx_0, p->value > 0.f);
	break;
    case Commands::Id::SET_CUT_REC_OFFSET:
	cut.setRecOffset(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_POSITION:
	cut.cutToPos(p->idx_0, p->value);
	break;
	// input filter
    case Commands::Id::SET_CUT_PRE_FILTER_FC:
	cut.setPreFilterFc(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_FC_MOD:
	cut.setPreFilterFcMod(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_RQ:
	cut.setPreFilterRq(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_LP:
	cut.setPreFilterLp(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_HP:
	cut.setPreFilterHp(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_BP:
	cut.setPreFilterBp(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_BR:
	cut.setPreFilterBr(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_DRY:
	cut.setPreFilterDry(p->idx_0, p->value);
	break;
	// -- output filter
    case Commands::Id::SET_CUT_POST_FILTER_FC:
	cut.setPostFilterFc(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_RQ:
	cut.setPostFilterRq(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_LP:
	cut.setPostFilterLp(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_HP:
	cut.setPostFilterHp(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_BP:
	cut.setPostFilterBp(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_BR:
	cut.setPostFilterBr(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_DRY:
	cut.setPostFilterDry(p->idx_0, p->value);
	break;

    case Commands::Id::SET_CUT_LEVEL_SLEW_TIME:
	outLevel[p->idx_0].setTime(p->value);
	break;
    case Commands::Id::SET_CUT_PAN_SLEW_TIME:
	outPan[p->idx_0].setTime(p->value);
	break;
    case Commands::Id::SET_CUT_RECPRE_SLEW_TIME:
	cut.setRecPreSlewTime(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_RATE_SLEW_TIME:
	cut.setRateSlewTime(p->idx_0, p->value);
	break;
    case Commands::Id::SET_CUT_VOICE_SYNC:
	cut.syncVoice(p->idx_0, p->idx_1, p->value);
	break;
    case Commands::Id::SET_CUT_BUFFER:
	cut.setVoiceBuffer(p->idx_0, buf[p->idx_1], BufFrames);
	break;
    default:;;
    }
}

void crone::SoftcutClient::reset() {
    for (int v = 0; v < NumVoices; ++v) {
        cut.setVoiceBuffer(v, buf[v%2], BufFrames);
        outLevel[v].setTarget(0.f);
        outLevel->setTime(0.001);
        outPan[v].setTarget(0.5f);
        outPan->setTime(0.001);

        enabled[v] = false;

        setPhaseQuant(v, 1.f);
        setPhaseOffset(v, 0.f);

        for (int i=0; i<2; ++i) {
            inLevel[i][v].setTime(0.001);
            inLevel[i][v].setTarget(0.0);
        }

        for (int w=0; w<NumVoices; ++w) {
            fbLevel[v][w].setTime(0.001);
            fbLevel[v][w].setTarget(0.0);
        }

        cut.setLoopStart(v, v*2);
        cut.setLoopEnd(v, v*2 + 1);

        output[v].clear();
        input[v].clear();
    }
    cut.reset();
}
