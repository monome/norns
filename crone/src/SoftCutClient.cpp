//
// Created by emb on 11/28/18.
//

#include <sndfile.hh>

#include "SoftCutClient.h"
#include "Commands.h"


// clamp unsigned int to upper bound, inclusive
static inline void clamp(size_t &x, const size_t a) {
    if (x > a) { x = a; }
}

crone::SoftCutClient::SoftCutClient() : Client<2, 2>("softcut") {

    for(int i=0; i<NumVoices; ++i) {
        cut.setVoiceBuffer(i, buf[i&1], BufFrames);
    }
}

void crone::SoftCutClient::process(jack_nframes_t numFrames) {
    Commands::softcutCommands.handlePending(this);
    clearBusses(numFrames);
    mixInput(numFrames);
    // process softcuts (overwrites output bus)
    for(int v=0; v<NumVoices; ++v) {
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


void crone::SoftCutClient::clearBusses(size_t numFrames) {
    mix.clear(numFrames);
    for (auto &b : input) { b.clear(numFrames); }
}

void crone::SoftCutClient::mixInput(size_t numFrames) {
    for(int ch=0; ch<2; ++ch) {
        for(int v=0; v<NumVoices; ++v) {
            if(cut.getRecFlag(v)) {
                input[v].mixFrom(&source[SourceAdc][ch], numFrames, inLevel[ch][v]);
                for (int w = 0; w < NumVoices; ++w) {
                    if(cut.getPlayFlag(w)) {
                        input[v].mixFrom(output[w], numFrames, fbLevel[v][w]);
                    }
                }
            }
        }
    }
}

void crone::SoftCutClient::mixOutput(size_t numFrames) {
    for(int v=0; v<NumVoices; ++v) {
        if(cut.getPlayFlag(v)) {
            mix.panMixFrom(output[v], numFrames, outLevel[v], outPan[v]);
        }
    }
}

void crone::SoftCutClient::handleCommand(Commands::CommandPacket *p) {
    switch(p->id) {
            //-- softcut routing
        case Commands::Id::SET_ENABLED_CUT:
	    std::cout << "softcut: setting enabled: voice "
		      << p->idx_0 << ": " << (p->value > 0) << std::endl;
            enabled[p->idx_0] = p->value > 0.f;
            break;
        case Commands::Id::SET_LEVEL_CUT:
	    	    std::cout << "softcut: setting voice output level "
		      << p->idx_0 << ": " << p->value << std::endl;
            outLevel[p->idx_0].setTarget(p->value);
            break;;
        case Commands::Id::SET_PAN_CUT:
            outPan[p->idx_0].setTarget(p->value);
            break;
        case Commands::Id::SET_LEVEL_IN_CUT:
	    std::cout << "softcut: setting voice input level "
		      << p->idx_0 << ": " << p->idx_1 << ": " << p->value << std::endl;
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
        case Commands::Id::SET_CUT_FILTER_FC:
            cut.setFilterFc(p->idx_0, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_FC_MOD:
            cut.setFilterFcMod(p->idx_0, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_RQ:
            cut.setFilterRq(p->idx_0, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_LP:
            cut.setFilterLp(p->idx_0, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_HP:
            cut.setFilterHp(p->idx_0, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_BP:
            cut.setFilterBp(p->idx_0, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_BR:
            cut.setFilterBr(p->idx_0, p->value);
            break;
        case Commands::Id::SET_CUT_FILTER_DRY:
            cut.setFilterDry(p->idx_0, p->value);
            break;
        case Commands::Id::SET_CUT_LEVEL_SLEW_TIME:
            cut.setLevelSlewTime(p->idx_0, p->value);
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
        default:
            ;;
    }
}

void crone::SoftCutClient::clearBuffer(int bufIdx, float start, float dur) {
    size_t frA =secToFrame(start);
    clamp(frA, BufFrames-1);
    size_t frB = frA + secToFrame(dur);
    clamp(frB, BufFrames);
    for(size_t i=frA; i<frB; ++i) { buf[bufIdx][i] = 0.f; }
}


/// FIXME: DRY this up
void crone::SoftCutClient::loadFileMono(const std::string &path, float startTimeSrc, float startTimeDst, float dur,
        int chanSrc, int chanDst) {

    SndfileHandle file(path);
    // FIXME: bail here if fail to open
    
    size_t frSrc = secToFrame(startTimeSrc);
    clamp(frSrc, BufFrames-1);

    size_t frDst = secToFrame(startTimeDst);
    clamp(frDst, BufFrames-1);

    size_t frDur;
    if (dur < 0.f) {
	auto maxDurSrc = file.frames() - frSrc;
	auto maxDurDst = file.frames() - frDst;
	frDur = maxDurSrc > maxDurDst ? maxDurDst : maxDurSrc;
    } else {
        frDur = secToFrame(dur);
    }

    auto numSrcChan = file.channels();
    std::unique_ptr<float[]> frBuf(new float[numSrcChan]);
    chanSrc = std::min(numSrcChan-1, std::max(0, chanSrc));
    chanDst = std::min(1, std::max(0, chanDst));

    for (size_t fr=0; fr<frDur; ++fr) {
        // FIXME: don't seek every frame with libsndfile?
        file.seek(frSrc, SEEK_SET);
        file.read(frBuf.get(), numSrcChan);
        buf[chanDst][frDst] = frBuf[chanSrc];
        ++frDst;
        ++frSrc;
        if (frDst >= BufFrames) {
            std::cerr << "SoftCutClient::loadFileMono(): exceeded buffer size; aborting" << std::endl;
            return;
        }
    }
}

/// FIXME: DRY this up
void crone::SoftCutClient::loadFileStereo(const std::string &path, float startTimeSrc, float startTimeDst, float dur) {
    SndfileHandle file(path);
    // FIXME: bail here if fail to open

    size_t frSrc = secToFrame(startTimeSrc);
    clamp(frSrc, BufFrames-1);

    size_t frDst = secToFrame(startTimeDst);
    clamp(frDst, BufFrames-1);

    size_t frDur;
    if (dur < 0.f) {
        auto maxDurSrc = file.frames() - frSrc;
        auto maxDurDst = file.frames() - frDst;
        frDur = maxDurSrc > maxDurDst ? maxDurDst : maxDurSrc;
    } else {
        frDur = secToFrame(dur);
    }

    auto numSrcChan = file.channels();
    if(numSrcChan<2) {
        std::cerr << "SoftCutClient::loadFileStereo(): not enough channels in source; aborting" << std::endl;
        return;
    }
    std::unique_ptr<float[]> frBuf(new float[numSrcChan]);

    for (size_t fr=0; fr<frDur; ++fr) {
        // FIXME: don't seek every frame with libsndfile?
        file.seek(frSrc, SEEK_SET);
        file.read(frBuf.get(), numSrcChan);
        buf[0][frDst] = frBuf[0];
        buf[1][frDst] = frBuf[1];
        ++frDst;
        ++frSrc;
        if (frDst >= BufFrames) {
            std::cerr << "SoftCutClient::loadFileStereo(): exceeded buffer size; aborting" << std::endl;
            return;
        }
    }
}
