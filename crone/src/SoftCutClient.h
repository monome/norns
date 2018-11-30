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
        enum { MAX_BUF_SIZE = 2048, NUM_VOICES = 2 };
        typedef enum { SOURCE_ADC=0 } SourceId;
        typedef Bus<2, MAX_BUF_SIZE> StereoBus;
        typedef Bus<1, MAX_BUF_SIZE> MonoBus;
    public:
        SoftCutClient();

    private:
        softcut::SoftCut<NUM_VOICES> cut;
        // busses
        StereoBus mix;
        MonoBus input[NUM_VOICES];
        MonoBus output[NUM_VOICES];
        // levels
        LogRamp inLevel[2][NUM_VOICES];
        LogRamp outLevel[NUM_VOICES];
        LogRamp outPan[NUM_VOICES];
        LogRamp fbLevel[NUM_VOICES][NUM_VOICES];
        // enabled flags
        bool enabled[NUM_VOICES];

    private:
        void process(jack_nframes_t numFrames) override;
        void setSampleRate(jack_nframes_t) override;
    public:
        void handleCommand(Commands::CommandPacket *p) override;
    private:
        void clearBusses(size_t numFrames);
        void mixInput(size_t numFrames);
        void mixOutput(size_t numFrames);
    };
}


#endif //CRONE_CUTCLIENT_H
