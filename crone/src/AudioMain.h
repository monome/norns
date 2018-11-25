//
// Created by emb on 11/19/18.
//

#ifndef CRONE_AUDIOMAIN_H
#define CRONE_AUDIOMAIN_H

#include "Bus.h"
#include "Commands.h"
#include "effects/StereoCompressor.h"
#include "effects/ZitaReverb.h"


#include "softcut/SoftCutVoice.h"
#include "softcut/SoftCut.h"

namespace  crone {

    class AudioMain {
        friend class Commands;
        enum {
            MAX_BUF_SIZE = 2048,
            SOFTCUT_COUNT = 2,
        };
    public:
        AudioMain();
        AudioMain(int sampleRate);

        void processBlock(
                const float *in_adc[2],
                const float *in_ext[2],
                float *out[2],
                size_t numFrames);
      
      void setDefaultParams();
      
    protected:
        void handleCommand(Commands::CommandPacket *p);
	
    private:
        void init(int sampleRate);
        void clearBusses(size_t numFrames);
        void processFx(size_t numFrames);
        void processSoftCut(size_t numFrames);

    private:
        // processors
        StereoCompressor comp;
        ZitaReverb reverb;
        softcut::SoftCut<SOFTCUT_COUNT> cut;
        // busses
        typedef Bus<2, MAX_BUF_SIZE> StereoBus;
        typedef Bus<1, MAX_BUF_SIZE> MonoBus;
        struct BusList {
            StereoBus adc_out;
            StereoBus ext_out;
            StereoBus dac_in;
            StereoBus ins_in;
            StereoBus ins_out;
            StereoBus aux_in;
            StereoBus aux_out;
            StereoBus adc_monitor;
            MonoBus cut_in[SOFTCUT_COUNT];
            MonoBus cut_out[SOFTCUT_COUNT];
            StereoBus cut_mix;
            BusList();
        };
        BusList bus;

        // levels
        struct SmoothLevelList{
        public:
            LogRamp adc;
            LogRamp dac;
            LogRamp ext;
            LogRamp monitor;
            LogRamp ext_aux;
            LogRamp ins_mix;
            LogRamp monitor_aux;
            LogRamp aux;
            LogRamp cut[SOFTCUT_COUNT];
            LogRamp cut_pan[SOFTCUT_COUNT];
            LogRamp adc_cut[SOFTCUT_COUNT][2];
            LogRamp ext_cut[SOFTCUT_COUNT][2];
            LogRamp cut_fb[SOFTCUT_COUNT][SOFTCUT_COUNT];
            LogRamp cut_aux;
            SmoothLevelList();
        };
        SmoothLevelList smoothLevels;

        struct StaticLevelList {
            float monitor_mix[4];
            StaticLevelList();
        };
        StaticLevelList staticLevels;

        // other state
        struct EnabledList {
            bool reverb;
            bool comp;
            bool cut[SOFTCUT_COUNT];
            EnabledList();
        };
        EnabledList enabled;

    };
}

#endif //CRONE_AUDIOMAIN_H
