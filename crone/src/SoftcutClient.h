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
        enum { MaxBlockFrames = 2048};
        enum { BufFrames = 16777216 };
        enum { NumVoices = 6 };
        typedef enum { SourceAdc=0 } SourceId;
        typedef Bus<2, MaxBlockFrames> StereoBus;
        typedef Bus<1, MaxBlockFrames> MonoBus;
    public:
        SoftcutClient();

    private:
        // processors
        softcut::Softcut<NumVoices> cut;
        // main buffer
        float buf[2][BufFrames];
        // buffer index for use with BufDiskWorker
        int bufIdx[2];
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

    private:
        void process(jack_nframes_t numFrames) override;
        void setSampleRate(jack_nframes_t) override;
        inline size_t secToFrame(float sec) {
            return static_cast<size_t >(sec * jack_get_sample_rate(Client::client));
        }

    public:
        /// FIXME: the "commands" structure shouldn't really be necessary.
        /// should be able to refactor most/all parameters for atomic access.
        // called from audio thread
        void handleCommand(Commands::CommandPacket *p) override;

        // these accessors can be called from other threads, so don't need to go through the commands queue
        //-- buffer manipulation
        //-- time parameters are in seconds
        //-- negative 'dur' parameter reads/clears/writes as much as possible.
        void readBufferMono(const std::string &path, float startTimeSrc = 0.f, float startTimeDst = 0.f,
                            float dur = -1.f, int chanSrc = 0, int chanDst = 0) {
            BufDiskWorker::requestReadMono(bufIdx[chanDst], path, startTimeSrc, startTimeDst, dur, chanSrc);
        }

        void readBufferStereo(const std::string &path, float startTimeSrc = 0.f, float startTimeDst = 0.f,
                              float dur = -1.f) {
            BufDiskWorker::requestReadStereo(bufIdx[0], bufIdx[1], path, startTimeSrc, startTimeDst, dur);
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

        // check if quantized phase has changed for a given voice
        // returns true
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

	void reset();

    private:
        void clearBusses(size_t numFrames);
        void mixInput(size_t numFrames);
        void mixOutput(size_t numFrames);
    };
}


#endif //CRONE_CUTCLIENT_H
