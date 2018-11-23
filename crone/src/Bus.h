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

        // clear the first N frames in the bus
        constexpr void clear(size_t numFrames) {
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    buf[ch][fr] = 0.f;
                }
            }
        }

        // sum from bus, without amplitude scaling
        constexpr void sumFrom(BusT &b, size_t numFrames) {
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    buf[ch][fr] += b.buf[ch][fr];
                }
            }
        }

        // mix from bus, with fixed amplitude
        constexpr void mixFrom(BusT &b, size_t numFrames, float level) {
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    buf[ch][fr] += b.buf[ch][fr] * level;
                }
            }
        }


        // mix from bus, with smoothed amplitude
        void mixFrom(BusT &b, size_t numFrames, LogRamp &level) {
            float l;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] += b.buf[ch][fr] * l;
                }
            }
        }

        // mix from pointer array, with smoothed amplitude
        void mixFrom(const float *src[NumChannels], size_t numFrames, LogRamp &level) {
            float l;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] += src[ch][fr] * l;
                }
            }
        }

        // mix to pointer array, with smoothed amplitude
        void mixTo(float *dst[NumChannels], size_t numFrames, LogRamp &level) {
            float l;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    dst[ch][fr] = buf[ch][fr] * l;
                }
            }
        }

        // mix from stereo bus with 2x2 level matrix
        void stereoMixFrom(BusT &b, size_t numFrames, const float level[4]) {
            for(size_t fr=0; fr<numFrames; ++fr) {
                buf[0][fr] += b.buf[0][fr] * level[0] + b.buf[1][fr] * level[2];
                buf[1][fr] += b.buf[0][fr] * level[1] + b.buf[1][fr] * level[3];
            }
        }

        // mix from two busses with balance coefficient (linear)
        void xfade(BusT &a, BusT &b, size_t numFrames, LogRamp &level) {
            float x, y, c;
            for(size_t fr=0; fr<numFrames; ++fr) {
                c = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    x = a.buf[ch][fr];
                    y = b.buf[ch][fr];
                    buf[ch][fr] = x + (y-x) * c;
                }
            }
        }

        // mix from two busses with balance coefficient (equal power)
        void xfadeEpFrom(BusT &a, BusT &b, size_t numFrames, LogRamp &level) {
            float x, y, c;
            for(size_t fr=0; fr<numFrames; ++fr) {
                c = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    x = a.buf[ch][fr];
                    y = b.buf[ch][fr];
                    buf[ch][fr] = x * sinf(c * (float)M_PI_2) + y * sinf((1.f-c) * (float)M_PI_2);
                }
            }
        }

    };

}

#endif //CRONE_BUS_H