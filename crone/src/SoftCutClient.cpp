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
    size_t frA = secToFrame(start);
    clamp(frA, BufFrames-1);
    size_t frB;
    if (dur < 0) {
        frB = BufFrames;
    } else {
        frB = frA + secToFrame(dur);
    }
    clamp(frB, BufFrames);
    for(size_t i=frA; i<frB; ++i) {
        buf[bufIdx][i] = 0.f;
    }
}


///////////////////
/// FIXME: DRY these all up

void crone::SoftCutClient::readBufferMono(const std::string &path, float startTimeSrc, float startTimeDst, float dur,
                                          int chanSrc, int chanDst) {
    SndfileHandle file(path);

    if (file.frames() < 1) {
        std::cerr << "SoftCutClient::readBufferMono(): empty / missing file: " << path << std::endl;
        return;
    }

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
        // FIXME: don't seek every frame with libsndfile! it is slow
        file.seek(frSrc, SEEK_SET);
        file.read(frBuf.get(), numSrcChan);
        buf[chanDst][frDst] = frBuf[chanSrc];
        ++frSrc;
        if (++frDst >= BufFrames) {
            std::cerr << "SoftCutClient::readBufferMono(): exceeded buffer size; aborting" << std::endl;
            return;
        }
    }
}

void crone::SoftCutClient::readBufferStereo(const std::string &path, float startTimeSrc, float startTimeDst, float dur) {
    SndfileHandle file(path);

    if (file.frames() < 1) {
        std::cerr << "SoftCutClient::readBufferStereo(): empty / missing file: " << path << std::endl;
        return;
    }

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
        std::cerr << "SoftCutClient::readBufferStereo(): not enough channels in source; aborting" << std::endl;
        return;
    }
    std::unique_ptr<float[]> frBuf(new float[numSrcChan]);

    for (size_t fr=0; fr<frDur; ++fr) {
        // FIXME: don't seek every frame with libsndfile! it is slow
        file.seek(frSrc, SEEK_SET);
        file.read(frBuf.get(), numSrcChan);
        buf[0][frDst] = frBuf[0];
        buf[1][frDst] = frBuf[1];
        frSrc++;
        if (++frDst >= BufFrames) {
            std::cerr << "SoftCutClient::readBufferStereo(): exceeded buffer size; aborting" << std::endl;
            return;
        }
    }
}


void crone::SoftCutClient::writeBufferMono(const std::string &path, float start = 0, float dur = -1, int chan = 0) {
    // FIXME: use the cpp sndfile interface for tidiness..
    SF_INFO sf_info;
    SNDFILE *file;

    sf_info.samplerate = 48000;
    sf_info.channels = 1;
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

    if ((file = sf_open(path.c_str(), SFM_WRITE, &sf_info)) == NULL) {
        char errstr[256];
        sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
        std::cerr << "cannot open sndfile" << path << " for writing (" << errstr << ")" << std::endl;
        return;
    }

    std::cerr << "SoftCutClient::writeBufferMono(): opened file for writing: " << path << std::endl;

    size_t frSrc = secToFrame(start);
    clamp(frSrc, BufFrames-1);

    size_t frDur;
    if (dur < 0.f) {
        // FIXME: should check available disk space?
        frDur = BufFrames - frSrc;
    } else {
        frDur = secToFrame(dur);
    }
    /// FIXME: write frames in blocks
    /// for now, write one frame at a time... :/

    size_t nf=0;
    while (nf<frDur) {
        if (sf_writef_float(file, buf[chan] + frSrc, 1) != 1) {
            std::cerr << "SoftCutClient::writeBufferMono(): write failed (disk space?) after " << nf << " frames" << std::endl;
            break;
        }
        ++frSrc;
        ++nf;
    }

    sf_close(file);
    std::cerr << "SoftCutClient::writeBufferMono(): done; wrote " << nf << " frames" << std::endl;
}


void crone::SoftCutClient::writeBufferStereo(const std::string &path, float start = 0, float dur = -1) {
    SF_INFO sf_info;
    SNDFILE *file;

    sf_info.samplerate = 48000;
    sf_info.channels = 2;
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

    if ((file = sf_open(path.c_str(), SFM_WRITE, &sf_info)) == NULL) {
        char errstr[256];
        sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
        std::cerr << "cannot open sndfile" << path << " for writing (" << errstr << ")" << std::endl;
        return;
    }

    std::cerr << "SoftCutClient::writeBufferStereo(): opened file for writing: " << path << std::endl;

    float frameBuf[2];

    size_t frSrc = secToFrame(start);
    clamp(frSrc, BufFrames-1);

    size_t frDur;
    if (dur < 0.f) {
        // FIXME: should check available disk space?
        frDur = BufFrames - frSrc;
    } else {
        frDur = secToFrame(dur);
    }
    /// FIXME: write frames in blocks
    /// for now, write one frame at a time... :/
    size_t nf=0;
    while (nf<frDur) {
        frameBuf[0] = buf[0][frSrc];
        frameBuf[1] = buf[1][frSrc];
        if (sf_writef_float(file, frameBuf, 1) != 1) {
            std::cerr << "SoftCutClient::writeBufferStereo(): write failed (disk space?) after " << nf << " frames" << std::endl;
            break;
        }
        ++frSrc;
        ++nf;
    }

    sf_close(file);
    std::cerr << "SoftCutClient::writeBufferStereo(): done; wrote " << nf << " frames" << std::endl;
}


