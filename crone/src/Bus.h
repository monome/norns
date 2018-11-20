//
// Created by emb on 11/19/18.
//

#ifndef CRONE_BUS_H
#define CRONE_BUS_H

#include "Utilities.h"

namespace  crone {

    template<size_t NumChannels, size_t BlockSize>
    class Bus {
    private:
        typedef Bus<NumChannels, BlockSize> BusT;
    public:
        float buf[NumChannels][BlockSize];

        constexpr void clear() {
            for(int ch=0; ch<NumChannels; ++ch) {
                for(int fr=0; fr<BlockSize; ++fr) {
                    buf[ch][fr] = 0.f;
                }
            }
        }

        constexpr void sumFrom(BusT &b) {
            for(int ch=0; ch<NumChannels; ++ch) {
                for(int fr=0; fr<BlockSize; ++fr) {
                    buf[ch][fr] += b.buf[ch][fr];
                }
            }
        }

        constexpr void mixFrom(BusT &b, float level) {
            for(int fr=0; fr<BlockSize; ++fr) {
                for(int ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] += b.buf[ch][fr] * level;
                }
            }
        }


        constexpr void mixFrom(BusT &b, LogRamp &level) {
            for(int fr=0; fr<BlockSize; ++fr) {
                for(int ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] += b.buf[ch][fr] * level.update();
                }
            }
        }

        constexpr void mixFrom(const float **src, LogRamp &level) {
            for(int fr=0; fr<BlockSize; ++fr) {
                for(int ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] += src[ch][fr] * level.update();
                }
            }
        }

        constexpr void stereoMixFrom(BusT &b, float l00, float l01, float l10, float l11) {
            for(int fr=0; fr<BlockSize; ++fr) {
                buf[0][fr] += b.buf[0][fr] * l00 + b.buf[1][fr] * l10;
                buf[1][fr] += b.buf[0][fr] * l01 + b.buf[1][fr] * l11;
            }
        }
    };

}

#endif //CRONE_BUS_H