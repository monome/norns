//
// Created by ezra on 11/3/18.
//

#ifndef CRONE_COMMANDS_H
#define CRONE_COMMANDS_H

#include <boost/lockfree/spsc_queue.hpp>


namespace crone {

    class MixerClient;
    class SoftCutClient;

    class Commands {
    public:
        typedef enum {
            //-- level commands
            SET_LEVEL_ADC,
            SET_LEVEL_DAC,
            SET_LEVEL_EXT,
            SET_LEVEL_EXT_AUX,
            SET_LEVEL_AUX_DAC,
            SET_LEVEL_MONITOR,
            SET_LEVEL_MONITOR_MIX,
            SET_LEVEL_MONITOR_AUX,
            SET_LEVEL_INS_MIX,

            SET_PARAM_REVERB,
            SET_PARAM_COMPRESSOR,

            SET_ENABLED_REVERB,
            SET_ENABLED_COMPRESSOR,

            // level of stereo ext out -> stereo cut in
            SET_LEVEL_EXT_CUT,
            // level of stereo ADC out -> stereo cut in
            SET_LEVEL_ADC_CUT,

            //-- softcut commands

            // mix
            SET_ENABLED_CUT,
            SET_LEVEL_CUT,
            SET_PAN_CUT,
            SET_LEVEL_CUT_AUX,
            // level of individual input channel -> cut voice
            // (separate commands just to avoid a 3rd parameter)
            SET_LEVEL_INPUT_0_CUT,
            SET_LEVEL_INPUT_1_CUT,

            // params
            SET_CUT_RATE,
            SET_CUT_LOOP_START,
            SET_CUT_LOOP_END,
            SET_CUT_LOOP_FLAG,
            SET_CUT_FADE_TIME,
            SET_CUT_REC_LEVEL,
            SET_CUT_PRE_LEVEL,
            SET_CUT_REC_FLAG,
            SET_CUT_REC_OFFSET,
            SET_CUT_POSITION,
            SET_CUT_FILTER_FC,
            SET_CUT_FILTER_FC_MOD,
            SET_CUT_FILTER_RQ,
            SET_CUT_FILTER_LP,
            SET_CUT_FILTER_HP,
            SET_CUT_FILTER_BP,
            SET_CUT_FILTER_BR,
            SET_CUT_FILTER_DRY,
            SET_CUT_LEVEL_SLEW_TIME,
            SET_CUT_RATE_SLEW_TIME,

            NUM_COMMANDS
        } Id;

    public:
        Commands();
        void post(Commands::Id id, float value);
        void post(Commands::Id id, int voice, float value);

        // FIXME: i guess things would be cleaner with a non-templated Client base/interface class
        void handlePending(MixerClient *client);
        void handlePending(SoftCutClient *client);

        struct CommandPacket {
            CommandPacket() = default;
            CommandPacket(Commands::Id i, int v, float f) : id(i), voice(v), value(f) {}
            Id id;
            int voice;
            float value;
        };

        static Commands mixerCommands;
        static Commands softcutCommands;

    private:
        boost::lockfree::spsc_queue <CommandPacket,
                boost::lockfree::capacity<200> > q;
    };

}

#endif //CRONE_COMMANDS_H
