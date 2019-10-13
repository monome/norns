//
// Created by emb on 11/19/18.
//

#ifndef CRONE_BUS_H
#define CRONE_BUS_H

#include <boost/assert.hpp>
#include "Utilities.h"

namespace  crone {

    template<size_t NumChannels, size_t BlockSize>
    class Bus {
    private:
        typedef Bus<NumChannels, BlockSize> BusT;
    public:
        float buf[NumChannels][BlockSize];

        // clear the entire bus
         void clear() {
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<BlockSize; ++fr) {
                    buf[ch][fr] = 0.f;
                }
            }
        }

        // clear the first N frames in the bus
         void clear(size_t numFrames) {
            BOOST_ASSERT(numFrames < BlockSize);
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    buf[ch][fr] = 0.f;
                }
            }
        }

        // copy from bus, with no scaling (overwrites previous contents)
        void copyFrom(Bus &b, size_t numFrames) {
            BOOST_ASSERT(numFrames < BlockSize);
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    buf[ch][fr] = b.buf[ch][fr];
                }
            }
         }

        // copy from bus to pointer array, with no scaling (overwrites previous contents)
        void copyTo(float *dst[NumChannels], size_t numFrames) {
            BOOST_ASSERT(numFrames < BlockSize);
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    dst[ch][fr] = buf[ch][fr];
                }
            }
        }


        // sum from bus, without amplitude scaling
         void addFrom(BusT &b, size_t numFrames) {
            BOOST_ASSERT(numFrames < BlockSize);
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    buf[ch][fr] += b.buf[ch][fr];
                }
            }
        }

        // mix from bus, with fixed amplitude
         void mixFrom(BusT &b, size_t numFrames, float level) {
            BOOST_ASSERT(numFrames < BlockSize);
            for(size_t ch=0; ch<NumChannels; ++ch) {
                for(size_t fr=0; fr<numFrames; ++fr) {
                    buf[ch][fr] += b.buf[ch][fr] * level;
                }
            }
        }


        // mix from bus, with smoothed amplitude
        void mixFrom(BusT &b, size_t numFrames, LogRamp &level) {
            BOOST_ASSERT(numFrames < BlockSize);
            float l;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] += b.buf[ch][fr] * l;
                }
            }
        }

        // apply smoothed amplitude
        void applyGain(size_t numFrames, LogRamp &level) {
            BOOST_ASSERT(numFrames < BlockSize);
            float l;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] *= l;
                }
            }
         }

        // mix from pointer array, with smoothed amplitude
        void mixFrom(const float *src[NumChannels], size_t numFrames, LogRamp &level) {
            BOOST_ASSERT(numFrames < BlockSize);
            float l;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] += src[ch][fr] * l;
                }
            }
        }

        // set from pointer array, with smoothed amplitude
        void setFrom(const float *src[NumChannels], size_t numFrames, LogRamp &level) {
            BOOST_ASSERT(numFrames < BlockSize);
            float l;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update();
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] = src[ch][fr] * l;
                }
            }
        }

        // set from pointer array, without scaling
        void setFrom(const float *src[NumChannels], size_t numFrames) {
            BOOST_ASSERT(numFrames < BlockSize);
            for(size_t fr=0; fr<numFrames; ++fr) {
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    buf[ch][fr] = src[ch][fr];
                }
            }
        }

        // mix to pointer array, with smoothed amplitude
        void mixTo(float *dst[NumChannels], size_t numFrames, LogRamp &level) {
            BOOST_ASSERT(numFrames < BlockSize);
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
            BOOST_ASSERT(numFrames < BlockSize);
            for (size_t fr = 0; fr < numFrames; ++fr) {
                buf[0][fr] += b.buf[0][fr] * level[0] + b.buf[1][fr] * level[2];
                buf[1][fr] += b.buf[0][fr] * level[1] + b.buf[1][fr] * level[3];
            }
        }

        // mix from two busses with balance coefficient (linear)
        void xfade(BusT &a, BusT &b, size_t numFrames, LogRamp &level) {
            BOOST_ASSERT(numFrames < BlockSize);
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
        void xfadeEp(BusT &a, BusT &b, size_t numFrames, LogRamp &level) {
            BOOST_ASSERT(numFrames < BlockSize);
            float x, y, l, c, d;
            for(size_t fr=0; fr<numFrames; ++fr) {
                l = level.update() * (float)M_PI_2;
                c = sinf(l);
                d = cosf(l);
                for(size_t ch=0; ch<NumChannels; ++ch) {
                    x = a.buf[ch][fr];
                    y = b.buf[ch][fr];
                    buf[ch][fr] = x * c + y * d;
                }
            }
        }

        // mix from mono->stereo bus, with level and pan (linear)
        void panMixFrom(Bus<1, BlockSize> a, size_t numFrames, LogRamp &level, LogRamp& pan) {
            BOOST_ASSERT(numFrames < BlockSize);
            static_assert(NumChannels > 1, "using panMixFrom() on mono bus");
            float l, c, x;
            for(size_t fr=0; fr<numFrames; ++fr) {
                x = a.buf[0][fr];
                l = level.update();
                c = pan.update();
                buf[0][fr] += x*l*(1.f-c);
                buf[1][fr] += x*l*c;
            }
        }


        // mix from mono->stereo bus, with level and pan (equal power)
        void panMixEpFrom(Bus<1, BlockSize> a, size_t numFrames, LogRamp &level, LogRamp& pan) {
            BOOST_ASSERT(numFrames < BlockSize);
            static_assert(NumChannels > 1, "using panMixFrom() on mono bus");
            float l, c, x;
            for(size_t fr=0; fr<numFrames; ++fr) {
                x = a.buf[0][fr];
                l = level.update();
                c = pan.update();
                c *= (float)M_PI_2;
                buf[0][fr] += x*l * cosf(c);
                buf[1][fr] += x*l * sinf(c);
            }
        }


    };


}

#endif //CRONE_BUS_H
