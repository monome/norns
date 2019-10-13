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
            SET_LEVEL_IN_CUT,
            SET_LEVEL_CUT_CUT,
            SET_LEVEL_TAPE,
            SET_LEVEL_TAPE_AUX,
            SET_LEVEL_TAPE_CUT,

            // params
            SET_CUT_REC_FLAG,
            SET_CUT_PLAY_FLAG,

            SET_CUT_RATE,
            SET_CUT_LOOP_START,
            SET_CUT_LOOP_END,
            SET_CUT_LOOP_FLAG,
            SET_CUT_POSITION,

            SET_CUT_FADE_TIME,
            SET_CUT_REC_LEVEL,
            SET_CUT_PRE_LEVEL,
            SET_CUT_REC_OFFSET,

            SET_CUT_PRE_FILTER_FC,
            SET_CUT_PRE_FILTER_FC_MOD,
            SET_CUT_PRE_FILTER_RQ,
            SET_CUT_PRE_FILTER_LP,
            SET_CUT_PRE_FILTER_HP,
            SET_CUT_PRE_FILTER_BP,
            SET_CUT_PRE_FILTER_BR,
            SET_CUT_PRE_FILTER_DRY,

	        SET_CUT_POST_FILTER_FC,
	        SET_CUT_POST_FILTER_RQ,
            SET_CUT_POST_FILTER_LP,
            SET_CUT_POST_FILTER_HP,
            SET_CUT_POST_FILTER_BP,
            SET_CUT_POST_FILTER_BR,
            SET_CUT_POST_FILTER_DRY,

            SET_CUT_LEVEL_SLEW_TIME,
            SET_CUT_PAN_SLEW_TIME,
            SET_CUT_RECPRE_SLEW_TIME,
            SET_CUT_RATE_SLEW_TIME,
            SET_CUT_VOICE_SYNC,
            SET_CUT_BUFFER,
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
        void handlePending(SoftCutClient *client);

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
