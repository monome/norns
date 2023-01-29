//
// Created by emb on 11/28/18.
//

#ifndef CRONE_CUTCLIENT_H
#define CRONE_CUTCLIENT_H

#include <iostream>

#include "BufDiskWorker.h"
#include "Bus.h"
#include "Client.h"
#include "Utilities.h"
#include "softcut/Softcut.h"
#include "softcut/Types.h"


namespace crone {
    class SoftcutClient: public Client<2, 2> {
    public:
        static constexpr float MaxRate = static_cast<float>(softcut::Resampler::OUT_BUF_FRAMES);
        static constexpr float MinRate = static_cast<float>(softcut::Resampler::OUT_BUF_FRAMES * -1);
        enum { MaxBlockFrames = 2048};
        enum { BufFrames = 16777216 };
        enum { NumVoices = 6 };
	enum { NumBuffers = 2 };
        typedef enum { SourceAdc=0 } SourceId;
        typedef Bus<2, MaxBlockFrames> StereoBus;
        typedef Bus<1, MaxBlockFrames> MonoBus;
    public:
        SoftcutClient();

    private:
        // processors
        softcut::Softcut<NumVoices> cut;
        // main buffer
        float buf[NumBuffers][BufFrames];
        // buffer index for use with BufDiskWorker
        int bufIdx[NumBuffers];
        // busses
        StereoBus mix;
        MonoBus input[NumVoices];
        MonoBus output[NumVoices];
        // levels
        LogRamp inLevel[2][NumVoices];
        LogRamp outLevel[NumVoices];
        LogRamp outPan[NumVoices];
        LogRamp fbLevel[NumVoices][NumVoices];
        // enabled flags
        bool enabled[NumVoices];
        softcut::phase_t quantPhase[NumVoices];
	float bufDur;

    private:
        void process(jack_nframes_t numFrames) override;
        void setSampleRate(jack_nframes_t) override;
        inline size_t secToFrame(float sec) {
            return static_cast<size_t >(sec * jack_get_sample_rate(Client::client));
        }

    public:
        // called from audio thread
        void handleCommand(Commands::CommandPacket *p) override;

        // these accessors can be called from other threads, so don't need to go through the commands queue
        //-- buffer manipulation
        //-- time parameters are in seconds
        //-- negative 'dur' parameter reads/clears/writes as much as possible.
        void readBufferMono(const std::string &path, float startTimeSrc = 0.f, float startTimeDst = 0.f,
                            float dur = -1.f, int chanSrc = 0, int chanDst = 0, float preserve = 0.f, float mix = 1.f) {
            BufDiskWorker::requestReadMono(bufIdx[chanDst], path, startTimeSrc, startTimeDst, dur, chanSrc, preserve, mix);
        }

        void readBufferStereo(const std::string &path, float startTimeSrc = 0.f, float startTimeDst = 0.f,
                              float dur = -1.f, float preserve = 0.f, float mix = 1.f) {
            BufDiskWorker::requestReadStereo(bufIdx[0], bufIdx[1], path, startTimeSrc, startTimeDst, dur, preserve, mix);
        }

        void writeBufferMono(const std::string &path, float start, float dur, int chan) {
            BufDiskWorker::requestWriteMono(bufIdx[chan], path, start, dur);

        }

        void writeBufferStereo(const std::string &path, float start, float dur) {
            BufDiskWorker::requestWriteStereo(bufIdx[0], bufIdx[1], path, start, dur);
        }

        void clearBuffer(int chan, float start=0.f, float dur=-1) {
            if (chan < 0 || chan > 1) { return; }
            BufDiskWorker::requestClear(bufIdx[chan], start, dur);
        }

        void clearBufferWithFade(int chan, float start=0.f, float dur=-1, float fadeTime=0.f, float preserve=0.f) {
            if (chan < 0 || chan > 1) { return; }
            BufDiskWorker::requestClearWithFade(bufIdx[chan], start, dur, fadeTime, preserve);
        }

        void copyBuffer(int srcChan, int dstChan,
                        float srcStart=0.f, float dstStart=0.f, float dur=-1,
                        float fadeTime=0.f, float preserve=0.f, bool reverse=false) {
            if (srcChan < 0 || srcChan > 1 || dstChan < 0 || dstChan > 1) { return; }
            BufDiskWorker::requestCopy(bufIdx[srcChan], bufIdx[dstChan],
                                       srcStart, dstStart, dur,
                                       fadeTime, preserve, reverse);
        }

        void renderSamples(int chan, float start, float dur, int count, BufDiskWorker::RenderCallback callback) {
            if (chan < 0 || chan > 1 || count < 1) { return; }
            BufDiskWorker::requestRender(bufIdx[chan], start, dur, count, callback);
        }
        
        void processBuffer(int chan, float start, float dur, BufDiskWorker::ProcessCallback callback) {
            if (chan < 0 || chan > 1) { return; }
            BufDiskWorker::requestProcess(chan, start, dur, callback);
        }
        
        void pokeBuffer(int chan, float start, float dur, BufDiskWorker::DoneCallback doneCallback) {
            if (chan < 0 || chan > 1) { return; }
            BufDiskWorker::requestPoke(chan, start, dur, doneCallback);
        }

        // check if quantized phase has changed for a given voice
        bool checkVoiceQuantPhase(int i) {
            if (quantPhase[i] != cut.getQuantPhase(i)) {
                quantPhase[i] = cut.getQuantPhase(i);
                return true;
            } else {
                return false;
            }
        }
	
        softcut::phase_t getQuantPhase(int i) {
            return cut.getQuantPhase(i);
        }

	void setPhaseQuant(int i, softcut::phase_t q) {
            cut.setPhaseQuant(i, q);
        }

	void setPhaseOffset(int i, float sec) {
            cut.setPhaseOffset(i, sec);
        }

        int getNumVoices() const { return NumVoices; }

        float getPosition(int i) {
           return cut.getSavedPosition(i);
        }

	void reset();

    private:
        void clearBusses(size_t numFrames);
        void mixInput(size_t numFrames);
        void mixOutput(size_t numFrames);
    };
}


#endif //CRONE_CUTCLIENT_H
