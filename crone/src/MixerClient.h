//
// Created by emb on 11/28/18.
//

#ifndef CRONE_MIXERCLIENT_H
#define CRONE_MIXERCLIENT_H

#include <atomic>

#include "Bus.h"
#include "Client.h"
#include "Tape.h"
#include "Utilities.h"

#include "effects/StereoCompressor.h"
#include "effects/ZitaReverb.h"


namespace  crone {
    class MixerClient: public Client<6, 6> {

    public:
        enum { MaxBufFrames = 2048 };
        typedef enum { SourceAdc=0, SourceCut=1, SourceExt=2 } SourceId;
        typedef enum { SinkDac=0, SinkCut=1, SinkExt=2 } SinkId;
        typedef Bus<2, MaxBufFrames> StereoBus;

    public:
        MixerClient();
        /// FIXME: the "commands" structure shouldn't really be necessary.
        /// should be able to refactor most/all parameters for atomic access.
        // called from audio thread
        void handleCommand(Commands::CommandPacket *p) override;
    private:
        void process(jack_nframes_t numFrames) override;
        void setSampleRate(jack_nframes_t) override;
    private:
        void processFx(size_t numFrames);
        void setFxDefaults();
    private:
        // processors
        StereoCompressor comp;
        ZitaReverb reverb;
        Tape<2> tape;

        // busses
        struct BusList {
            // sources
            StereoBus adc_source;
            StereoBus cut_source;
            StereoBus ext_source;
            // sinks
            StereoBus ext_sink;
            StereoBus cut_sink;
            StereoBus dac_sink;
            // fx I/O
            StereoBus ins_in;
            StereoBus ins_out;
            StereoBus aux_in;
            StereoBus aux_out;
            // monitor mix
            StereoBus adc_monitor;
            // tape playback
            StereoBus tape;
            BusList();
        };
        BusList bus;

        // levels
        struct SmoothLevelList{
        public:
            // "master" I/O levels
            LogRamp adc;
            LogRamp dac;
            LogRamp ext;
            LogRamp cut;
            LogRamp monitor;
            // softcut input levels
            LogRamp adc_cut;
            LogRamp ext_cut;
            // aux send levels
            LogRamp monitor_aux;
            LogRamp cut_aux;
            LogRamp ext_aux;
            // FX return / mix levels
            LogRamp aux;
            LogRamp ins_mix;
            // tape playback level
            LogRamp tape;
            SmoothLevelList();
            void setSampleRate(float sr);
        };

        SmoothLevelList smoothLevels;

        struct StaticLevelList {
            // 2x2 matrix for monitor mix
            float monitor_mix[4];
            StaticLevelList();
        };
        StaticLevelList staticLevels;

        // other state
        struct EnabledList {
            bool reverb;
            bool comp;
            EnabledList();
        };
        EnabledList enabled;


    public:
        struct VuLevels {
            std::atomic<float> absPeakIn[2];
            std::atomic<float> absPeakOut[2];
            void clear();
            void update(StereoBus &in, StereoBus &out, size_t numFrames);
        };

        VuLevels vuLevels;
        VuLevels* getVuLevels() { return &vuLevels; }

        void openTapeRecord(const char* path) {
            tape.writer.open(path);
        }

        void startTapeRecord() {
            tape.writer.start();
        }

        void stopTapeRecord() {
            tape.writer.stop();
        }


        void openTapePlayback(const char* path) {
            tape.reader.open(path);
        }

        void startTapePlayback() {
            tape.reader.start();
        }

        void stopTapePlayback() {
            tape.reader.stop();
        }

    };
}

#endif //CRONE_MIXERCLIENT_H
