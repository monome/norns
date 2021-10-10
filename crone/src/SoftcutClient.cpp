//
// Created by emb on 11/28/18.
//

#include <sndfile.hh>

#include "BufDiskWorker.h"
#include "Commands.h"
#include "SoftcutClient.h"

// clamp to upper bound (unsigned int)
static inline void clamp(size_t &x, const size_t max) {
    if (x > max) { x = max; }
}

// clamp to upper bound (float)
static inline void clamp(float &x, const float max) {
    x = std::min(max, x);
}


// clamp to lower and upper bounds (float)
static inline void clamp(float &x, const float min, const float max) {
    x = std::max(min, std::min(max, x));
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
    bufDur = (float)BufFrames / sr;
    cut.setSampleRate(sr);
}


void crone::SoftcutClient::clearBusses(size_t numFrames) {
    mix.clear(numFrames);
    for (auto &b : input) { b.clear(numFrames); }
}

void crone::SoftcutClient::mixInput(size_t numFrames) {
    for (int dst = 0; dst < NumVoices; ++dst) {
        if (cut.getRecFlag(dst) && enabled[dst]) {
            for (int ch = 0; ch < 2; ++ch) {
                input[dst].mixFrom(&source[SourceAdc][ch], numFrames, inLevel[ch][dst]);
            }
            for (int src = 0; src < NumVoices; ++src) {
                if (cut.getPlayFlag(src) && enabled[src]) {
                    input[dst].mixFrom(output[src], numFrames, fbLevel[src][dst]);
                }
            }
        }
    }
}

void crone::SoftcutClient::mixOutput(size_t numFrames) {
    for (int v = 0; v < NumVoices; ++v) {
        if (cut.getPlayFlag(v) && enabled[v]) {
            mix.panMixEpFrom(output[v], numFrames, outLevel[v], outPan[v]);
        }
    }
}

void crone::SoftcutClient::handleCommand(Commands::CommandPacket *p) {
    size_t idx_0 = p->idx_0;
    size_t idx_1 = p->idx_1;
    float value = p->value;
    clamp(idx_0, NumVoices-1);
    switch (p->id) {
        //-- softcut routing
    case Commands::Id::SET_ENABLED_CUT:
	enabled[idx_0] = value > 0.f;
	if (!enabled[idx_0]) {
	    cut.stopVoice(idx_0);
	}
	break;
    case Commands::Id::SET_LEVEL_CUT:
	outLevel[idx_0].setTarget(value);
	break;;
    case Commands::Id::SET_PAN_CUT:
	clamp(value, -1.f, 1.f);
	outPan[idx_0].setTarget((value/2)+0.5);
	break;
    case Commands::Id::SET_LEVEL_IN_CUT:
	clamp(idx_1, NumVoices-1);
	inLevel[idx_0][idx_1].setTarget(value);
	break;
    case Commands::Id::SET_LEVEL_CUT_CUT:
	clamp(idx_1, NumVoices-1);
	fbLevel[idx_0][idx_1].setTarget(value);
	break;
	//-- softcut commands
    case Commands::Id::SET_CUT_RATE:
	clamp(value, MinRate, MaxRate);
	cut.setRate(idx_0, value);
	break;
    case Commands::Id::SET_CUT_LOOP_START:
	clamp(value, 0.f, bufDur);
	cut.setLoopStart(idx_0, value);
	break;
    case Commands::Id::SET_CUT_LOOP_END:
	clamp(value, 0.f, bufDur);
	cut.setLoopEnd(idx_0, value);
	break;
    case Commands::Id::SET_CUT_LOOP_FLAG:
	cut.setLoopFlag(idx_0, value > 0.f);
	break;
    case Commands::Id::SET_CUT_FADE_TIME:
	value = std::max(0.f, value);
	cut.setFadeTime(idx_0, value);
	break;
    case Commands::Id::SET_CUT_REC_LEVEL:
	cut.setRecLevel(idx_0, value);
	break;
    case Commands::Id::SET_CUT_PRE_LEVEL:
	cut.setPreLevel(idx_0, value);
	break;
    case Commands::Id::SET_CUT_REC_FLAG:
	cut.setRecFlag(idx_0, value > 0.f);
	break;
    case Commands::Id::SET_CUT_PLAY_FLAG:
	cut.setPlayFlag(idx_0, value > 0.f);
	break;
    case Commands::Id::SET_CUT_REC_OFFSET:
	cut.setRecOffset(idx_0, value);
	break;
    case Commands::Id::SET_CUT_POSITION:
	clamp(value, 0.f, bufDur);
	cut.cutToPos(idx_0, value);
	break;
	// input filter
    case Commands::Id::SET_CUT_PRE_FILTER_FC:
	clamp(value, 10.f, 12000.f);
	cut.setPreFilterFc(idx_0, value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_FC_MOD:
	clamp(value, 0.f, 1.f);
	cut.setPreFilterFcMod(idx_0, value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_RQ:
	clamp(value, 0.0001f, 20.f);
	cut.setPreFilterRq(idx_0, value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_LP:
	cut.setPreFilterLp(idx_0, value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_HP:
	cut.setPreFilterHp(idx_0, value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_BP:
	cut.setPreFilterBp(idx_0, value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_BR:
	cut.setPreFilterBr(idx_0, value);
	break;
    case Commands::Id::SET_CUT_PRE_FILTER_DRY:
	cut.setPreFilterDry(idx_0, value);
	break;
	// -- output filter
    case Commands::Id::SET_CUT_POST_FILTER_FC:
	clamp(value, 10.f, 12000.f);
	cut.setPostFilterFc(idx_0, value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_RQ:
	clamp(value, 0.0001f, 20.f);
	cut.setPostFilterRq(idx_0, value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_LP:
	cut.setPostFilterLp(idx_0, value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_HP:
	cut.setPostFilterHp(idx_0, value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_BP:
	cut.setPostFilterBp(idx_0, value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_BR:
	cut.setPostFilterBr(idx_0, value);
	break;
    case Commands::Id::SET_CUT_POST_FILTER_DRY:
	cut.setPostFilterDry(idx_0, value);
	break;

    case Commands::Id::SET_CUT_LEVEL_SLEW_TIME:
	value = std::max(0.f, value);
	outLevel[idx_0].setTime(value);
	break;
    case Commands::Id::SET_CUT_PAN_SLEW_TIME:
	value = std::max(0.f, value);
	outPan[idx_0].setTime(value);
	break;
    case Commands::Id::SET_CUT_RECPRE_SLEW_TIME:
	value = std::max(0.f, value);
	cut.setRecPreSlewTime(idx_0, value);
	break;
    case Commands::Id::SET_CUT_RATE_SLEW_TIME:
	value = std::max(0.f, value);
	cut.setRateSlewTime(idx_0, value);
	break;
    case Commands::Id::SET_CUT_VOICE_SYNC:
	clamp(idx_1, NumVoices-1);
	cut.syncVoice(idx_0, idx_1, value);
	break;
    case Commands::Id::SET_CUT_BUFFER:
	clamp(idx_1, NumBuffers - 1);
	cut.setVoiceBuffer(idx_0, buf[idx_1], BufFrames);
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
