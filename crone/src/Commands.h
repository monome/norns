//
// Created by ezra on 11/3/18.
//

#ifndef CRONE_COMMANDS_H
#define CRONE_COMMANDS_H

#include <boost/lockfree/spsc_queue.hpp>

namespace crone {

    class AudioMain;

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

            //-- softcut commands
            SET_SOFTCUT_RATE,
            SET_SOFTCUT_LOOP_START,
            SET_SOFTCUT_LOOP_END,
            SET_SOFTCUT_LOOP_FLAG,
            SET_SOFTCUT_FADE_TIME,
            SET_SOFTCUT_REC_LEVEL,
            SET_SOFTCUT_PRE_LEVEL,
            SET_SOFTCUT_REC_FLAG,
            SET_SOFTCUT_REC_OFFSET,
            SET_SOFTCUT_POSITION,
            SET_SOFTCUT_FILTER_FC,
            SET_SOFTCUT_FILTER_FC_MOD,
            SET_SOFTCUT_FILTER_RQ,
            SET_SOFTCUT_FILTER_LP,
            SET_SOFTCUT_FILTER_HP,
            SET_SOFTCUT_FILTER_BP,
            SET_SOFTCUT_FILTER_BR,
            SET_SOFTCUT_FILTER_DRY,
            SET_SOFTCUT_AMP_L,
            SET_SOFTCUT_AMP_R,
            SET_SOFTCUT_PRE_FADE_WINDOW,
            SET_SOFTCUT_REC_FADE_DELAY,
            SET_SOFTCUT_PRE_FADE_SHAPE,
            SET_SOFTCUT_REC_FADE_SHAPE,
            SET_SOFTCUT_LEVEL_SLEW_TIME,
            SET_SOFTCUT_RATE_SLEW_TIME,
            NUM_COMMANDS
        } Id;

    public:

        static void post(Commands::Id id, float value);
        static void post(Commands::Id id, int voice, float value);
        static void handlePending(AudioMain *audio);

        struct CommandPacket {
            CommandPacket() = default;
            CommandPacket(Commands::Id i, int v, float f) : id(i), voice(v), value(f) {}
            Id id;
            int voice;
            float value;
        };

    private:
        static boost::lockfree::spsc_queue <CommandPacket> q;
    };

}

#endif //CRONE_COMMANDS_H
