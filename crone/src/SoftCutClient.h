//
// Created by emb on 11/28/18.
//

#ifndef CRONE_CUTCLIENT_H
#define CRONE_CUTCLIENT_H

#include "Client.h"
#include "Bus.h"
#include "Utilities.h"
#include "softcut/SoftCut.h"

namespace crone {
    class SoftCutClient: public Client<2, 2> {
    public:
        enum { MaxBlockFrames = 2048, NumVoices = 2 };

        enum { BufFrames = 16777216 };
        typedef enum { SourceAdc=0 } SourceId;
        typedef Bus<2, MaxBlockFrames> StereoBus;
        typedef Bus<1, MaxBlockFrames> MonoBus;
    public:
        SoftCutClient();

    private:
        // processors
        softcut::SoftCut<NumVoices> cut;
        // main buffer
        float buf[BufFrames];
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

    private:
        void process(jack_nframes_t numFrames) override;
        void setSampleRate(jack_nframes_t) override;
    public:
        /// FIXME: the "commands" structure shouldn't really be necessary.
        /// should be able to refactor most/all parameters for atomic access.
        // called from audio thread
        void handleCommand(Commands::CommandPacket *p) override;
        // these accessors can be called from other threads, so don't need to go through the commands queue
        //-- buffer manipulation
        //-- time parameters are in seconds
        //-- negative 'dur' parameter reads/clears as much as possible.
        void loadFile(std::string path, float startTimeSrc=0.f, float startTimeDst=0.f, float dur=-1.f, int channel=0);
        void clearBuffer(float startTime=0.f, float dur=-1);
    private:
        void clearBusses(size_t numFrames);
        void mixInput(size_t numFrames);
        void mixOutput(size_t numFrames);
    };
}


#endif //CRONE_CUTCLIENT_H
