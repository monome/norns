//
// Created by ezra on 11/3/18.
//

#ifndef CRONE_COMMANDS_H
#define CRONE_COMMANDS_H

#include <boost/lockfree/spsc_queue.hpp>


namespace crone {

    class MixerClient;
    class SoftcutClient;

    class Commands {
    public:
        typedef enum {
            //-- level commands
            SET_LEVEL_ADC,
            SET_LEVEL_DAC,
            SET_LEVEL_EXT,
            SET_LEVEL_EXT_AUX,
	        SET_LEVEL_CUT_MASTER,
            SET_LEVEL_AUX_DAC,
            SET_LEVEL_MONITOR,
            SET_LEVEL_MONITOR_MIX,
            SET_LEVEL_MONITOR_AUX,
            SET_LEVEL_INS_MIX,

            SET_PARAM_REVERB,
            SET_PARAM_COMPRESSOR,

            SET_ENABLED_REVERB,
            SET_ENABLED_COMPRESSOR,

            //--- tape routing
            SET_LEVEL_TAPE,
            SET_LEVEL_TAPE_AUX,

            //-- softcut routing
            SET_LEVEL_EXT_CUT,
            SET_LEVEL_ADC_CUT,
            SET_LEVEL_TAPE_CUT,
            SET_LEVEL_CUT_AUX,

            //-- softcut commands

            // mix
            SET_ENABLED_CUT,
            SET_LEVEL_CUT,
            SET_PAN_CUT,
            // level of individual input channel -> cut voice
            // (separate commands just to avoid a 3rd parameter)
            SET_LEVEL_IN_CUT,
            SET_LEVEL_CUT_CUT,

            // voice parameters
            SET_CUT_VOICE_BUFFER,
            SET_CUT_VOICE_REC_ENABLED,
            SET_CUT_VOICE_PLAY_ENABLED,

            SET_CUT_VOICE_RATE,
            SET_CUT_VOICE_LOOP_START,
            SET_CUT_VOICE_LOOP_END,
            SET_CUT_VOICE_LOOP_ENABLED,
            SET_CUT_VOICE_POSITION,

            SET_CUT_VOICE_FADE_TIME,
            SET_CUT_VOICE_REC_LEVEL,
            SET_CUT_VOICE_PRE_LEVEL,
            SET_CUT_VOICE_REC_OFFSET,

            SET_CUT_VOICE_PRE_FILTER_FC,
            SET_CUT_VOICE_PRE_FILTER_FC_MOD,
            SET_CUT_VOICE_PRE_FILTER_Q,
            SET_CUT_VOICE_PRE_FILTER_ENABLED,

            SET_CUT_VOICE_POST_FILTER_FC,
            SET_CUT_VOICE_POST_FILTER_RQ,
            SET_CUT_VOICE_POST_FILTER_LP,
            SET_CUT_VOICE_POST_FILTER_HP,
            SET_CUT_VOICE_POST_FILTER_BP,
            SET_CUT_VOICE_POST_FILTER_BR,
            SET_CUT_VOICE_POST_FILTER_DRY,

            SET_CUT_VOICE_LEVEL_SLEW_TIME,
            SET_CUT_VOICE_PAN_SLEW_TIME,
            SET_CUT_VOICE_RECPRE_SLEW_TIME,
            SET_CUT_VOICE_RATE_SLEW_TIME,

            SET_CUT_VOICE_SYNC,
            SET_CUT_VOICE_DUCK_TARGET,
            SET_CUT_VOICE_FOLLOW_TARGET,

            NUM_COMMANDS,
        } Id;

    public:
        Commands();
        void post(Commands::Id id, float f);
        void post(Commands::Id id, int i, float f);
        void post(Commands::Id id, int i, int j);
        void post(Commands::Id id, int i, int j, float f);

        // FIXME: i guess things would be cleaner with a non-templated Client base/interface class
        void handlePending(MixerClient *client);
        void handlePending(SoftcutClient *client);

        struct CommandPacket {
            CommandPacket() = default;
            CommandPacket(Commands::Id i, int i0,  float f) : id(i), idx_0(i0), idx_1(-1), value(f) {}
            CommandPacket(Commands::Id i, int i0, int i1) : id(i), idx_0(i0), idx_1(i1) {}
            CommandPacket(Commands::Id i, int i0, int i1, float f) : id(i), idx_0(i0), idx_1(i1), value(f) {}
            Id id;
            int idx_0;
            int idx_1;
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
