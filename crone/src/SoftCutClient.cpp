//
// Created by emb on 11/28/18.
//

#include "SoftCutClient.h"
#include "Commands.h"

crone::SoftCutClient::SoftCutClient() : Client<2, 2>("softcut"), cut(buf, BufFrames) {}

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
            input[v].mixFrom(&source[SourceAdc][ch], numFrames, inLevel[ch][v]);
            //input[v].mixFrom(&source[SourceExt][ch], numFrames, in_ext[ch][v]);
            for(int w=0; w<NumVoices; ++w) {
                input[v].mixFrom(output[w], numFrames, fbLevel[v][w]);
            }
        }
    }
}

void crone::SoftCutClient::mixOutput(size_t numFrames) {
    for(int v=0; v<NumVoices; ++v) {
        mix.panMixFrom(output[v], numFrames, outLevel[v], outPan[v]);
    }
}

void crone::SoftCutClient::handleCommand(Commands::CommandPacket *p) {
    switch(p->id) {
            //-- softcut routing
        case Commands::Id::SET_ENABLED_CUT:
            enabled[p->idx_0] = p->value > 0.f;
            break;
        case Commands::Id::SET_LEVEL_CUT:
            outLevel[p->idx_0].setTarget(p->value);
            break;;
        case Commands::Id::SET_PAN_CUT:
            outPan[p->idx_0].setTarget(p->value);
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

        default:
            ;;
    }
}

void crone::SoftCutClient::clearBuffer(float startTime, float dur) {

}
