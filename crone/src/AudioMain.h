//
// Created by emb on 11/19/18.
//

#ifndef CRONE_AUDIOMAIN_H
#define CRONE_AUDIOMAIN_H

#include "Bus.h"

namespace  crone {
    class AudioMain {
        enum { MAX_BUF_SIZE = 512};
    private:
        typedef Bus<2, MAX_BUF_SIZE> StereoBus;
        struct {
            StereoBus adc_out;
            StereoBus dac_in;
            StereoBus ins_in;
            StereoBus ins_out;
            StereoBus aux_in;
            StereoBus aux_out;
            StereoBus adc_monitor;
        } bus;
        struct {
            LogRamp adc;
            LogRamp monitor;
            LogRamp dac;
        } smoothLevels;

        struct {
            float monitor_l_l;
            float monitor_l_r;
            float monitor_r_l;
            float monitor_r_r;
        } staticLevels;
    };
}

#endif //CRONE_AUDIOMAIN_H
