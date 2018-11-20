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

        // clear the entire bus
        constexpr void clear() {
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<BlockSize; ++fr) {
                    buf[ch][fr] = 0.f;
                }
            }
        }

        constexpr void clear(size_t numFrames) {
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    buf[ch][fr] = 0.f;
                }
            }
        }

        constexpr void sumFrom(BusT &b, size_t numFrames) {
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    buf[ch][fr] += b.buf[ch][fr];
                }
            }
        }

        constexpr void mixFrom(BusT &b, size_t numFrames, float level) {
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    buf[ch][fr] += b.buf[ch][fr] * level;
                }
            }
        }


        void mixFrom(BusT &b, size_t numFrames, LogRamp &level) {
            float l;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] += b.buf[ch][fr] * l;
                }
            }
        }

        // mix from array of pointers
        void mixFrom(const float *src[NumChannels], size_t numFrames, LogRamp &level) {
            float l;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] += src[ch][fr] * l;
                }
            }
        }


        // mix to array of pointers
        void mixTo(float *dst[NumChannels], size_t numFrames, LogRamp &level) {
            float l;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    dst[ch][fr] = buf[ch][fr] * l;
                }
            }
        }

        constexpr void stereoMixFrom(BusT &b, size_t numFrames, float l00, float l01, float l10, float l11) {
            for(size_t fr=0; fr<numFrames; ++fr) {
                buf[0][fr] += b.buf[0][fr] * l00 + b.buf[1][fr] * l10;
                buf[1][fr] += b.buf[0][fr] * l01 + b.buf[1][fr] * l11;
            }
        }
    };

}

#endif //CRONE_BUS_H