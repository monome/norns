//
// Created by emb on 12/01/18.
//

#ifndef CRONE_TAPE_H
#define CRONE_TAPE_H

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <jack/ringbuffer.h>
#include <jack/types.h>
#include <sndfile.h>
#include <string.h>

#include "Window.h"

namespace crone {

template <int NumChannels>
class Tape {
  private:
    typedef jack_default_audio_sample_t Sample;
    static constexpr size_t sampleSize = sizeof(Sample);
    static constexpr size_t frameSize = sampleSize * NumChannels;
    static constexpr size_t ringBufFrames = 16384;
    static constexpr size_t ringBufBytes = sampleSize * NumChannels * ringBufFrames;

  public:
    //-----------------------------------------------------------------------------------------------
    //-- base class for sound file access

    class SfStream {
      protected:
        SNDFILE *file;
        std::unique_ptr<std::thread> th;
        std::mutex mut;
        std::condition_variable cv;
        std::unique_ptr<jack_ringbuffer_t> ringBuf;
        volatile int status;

        typedef enum {
            Starting,
            Playing,
            Stopping,
            Stopped
        } EnvState;

        std::atomic<EnvState> envState;
        std::atomic<int> envIdx;

      public:
        std::atomic<bool> isRunning;
        std::atomic<bool> shouldStop;

      public:
        SfStream()
            : file(nullptr),
              status(0),
              isRunning(false),
              shouldStop(false) {
            ringBuf = std::unique_ptr<jack_ringbuffer_t>(jack_ringbuffer_create(ringBufBytes));
            envIdx = 0;
            envState = Stopped;
        }

        // from any thread
        virtual void start() {
            if (isRunning) {
                std::cout << "Tape::SfStream::start(): already running" << std::endl;
                return;
            } else {
                std::cout << "Tape::SfStream: starting..." << std::endl;
                envIdx = 0;
                envState = Starting;

                this->th = std::make_unique<std::thread>(
                    [this]() {
                        this->diskLoop();
                    });
                this->th->detach();
            }
        }

        // from any thread
        void stop() {
            envState = Stopping;
        }

      protected:
        virtual void diskLoop() = 0;

        float getEnvSample() {
            float y = 0.f;
            switch (envState) {
            case Starting:
                y = Window::raisedCosShort[envIdx];
                incEnv();
                break;
                ;
            case Stopping:
                y = Window::raisedCosShort[envIdx];
                decEnv();
                break;
            case Playing:
                y = 1.0;
                break;
            case Stopped:
            default:
                y = 0.f;
            }
            return y;
        }

      private:
        void incEnv() {
            envIdx++;
            if (envIdx >= static_cast<int>(Window::raisedCosShortLen)) {
                envIdx = Window::raisedCosShortLen - 1;
                envState = Playing;
            }
        }

        void decEnv() {
            envIdx--;
            if (envIdx < 0) {
                envIdx = 0;
                envState = Stopped;
                shouldStop = true;
                std::cout << "Tape: fade-out finished; stopping" << std::endl;
            }
        }
    };

    //--------------------------------------------------------------------------------------------------------------
    //---- Writer class

    class Writer : public SfStream {
        friend class Tape;

      private:
        static constexpr size_t maxFramesToWrite = ringBufFrames;
        static constexpr size_t minBytesToWrite = 2048; // totally arbitrary
        static constexpr size_t minFramesToWrite = minBytesToWrite * frameSize;
        bool dataPending;
        //  buffer for writing to soundfile (disk thread)
        Sample diskOutBuf[maxFramesToWrite * NumChannels];
        //  buffer for interleaving before ringbuf (audio thread)
        Sample pushBuf[maxFramesToWrite * NumChannels];
        size_t numFramesCaptured;
        size_t maxFrames;

      public:
        // call from audio thread
        void process(const float *src[NumChannels], size_t numFrames) {
            if (!SfStream::isRunning) {
                return;
            }

            // push to ringbuffer
            jack_ringbuffer_t *rb = this->ringBuf.get();
            size_t bytesToPush = numFrames * frameSize;
            const size_t bytesAvailable = jack_ringbuffer_write_space(rb);

            if (bytesToPush > bytesAvailable) {
#if 0 
                    std::cout << "Tape: writer overrun: " 
                              << bytesAvailable << " bytes available; " 
                              << bytesToPush << " bytes to push; "
                              << numFramesCaptured << " frames captured" 
                              << std::endl;
#endif
                // discard input if the ringbuffer is full;
                // this causes a dropout but hopefully nothing Really Bad
                numFrames = bytesAvailable / frameSize;
                bytesToPush = numFrames * frameSize;
            }

            /// libsndfile requires interleaved data. we do that here before pushing to ringbuf
            float *dst = pushBuf;
            for (size_t fr = 0; fr < numFrames; ++fr) {
                // while we're interleaving, also apply envelope
                float amp = SfStream::getEnvSample();
                for (int ch = 0; ch < NumChannels; ++ch) {

                    *dst++ = src[ch][fr] * amp;
                }
            }
            jack_ringbuffer_write(rb, (const char *)pushBuf, bytesToPush);
            this->dataPending = true;
            this->cv.notify_one();

            if (this->mut.try_lock()) {
                this->dataPending = true;
                this->cv.notify_one();
                this->mut.unlock();
            }
        }

        // call from disk thread
        void diskLoop() override {
            SfStream::isRunning = true;
            SfStream::shouldStop = false;
            numFramesCaptured = 0;
            size_t bytesAvailable;
            while (!SfStream::shouldStop) {
                {
                    std::unique_lock<std::mutex> lock(this->mut);
                    this->cv.wait(lock, [this] {
                        return this->dataPending;
                    });

                    // check for spurious wakeup
                    if (!dataPending) {
                        continue;
                    }
                }

                bytesAvailable = jack_ringbuffer_read_space(this->ringBuf.get());
                if (bytesAvailable < minBytesToWrite) {
                    {
                        std::unique_lock<std::mutex> lock(this->mut);
                        dataPending = false;
                    }
                    continue;
                }

                int framesToWrite = bytesAvailable / frameSize;

                if (framesToWrite > (int)maxFramesToWrite) {
                    // _really_ shouldn't happen
                    std::cout << "warning: Tape::Writer has too many frames to write" << std::endl;
                    framesToWrite = (int)maxFramesToWrite;
                }

                jack_ringbuffer_read(this->ringBuf.get(), (char *)diskOutBuf, framesToWrite * frameSize);
                // immediately signal audio thread that we're done with pending data
                {
                    std::unique_lock<std::mutex> lock(this->mut);
                    dataPending = false;
                }

                if (sf_writef_float(this->file, diskOutBuf, framesToWrite) != framesToWrite) {
                    char errstr[256];
                    sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
                    std::cout << "error: Tape::writer failed to write (libsndfile: " << errstr << ")" << std::endl;
                    this->status = EIO;
                    break;
                }

                numFramesCaptured += framesToWrite;
                if (numFramesCaptured >= maxFrames) {
                    std::cout << "Tape: writer exceeded max frame count; aborting.";
                    break;
                }
            }

            std::cout << "Tape::writer closing file...";
            sf_close(this->file);
            std::cout << " done." << std::endl;
            SfStream::isRunning = false;
        }

        // from any thread
        bool open(const std::string &path,
                  size_t maxFrames = JACK_MAX_FRAMES, // <-- ridiculous big number
                  int sampleRate = 48000,
                  int bitDepth = 24) {

            if (SfStream::isRunning) {
                std::cout << "Tape Writer::open(): stream is running; no action was taken" << std::endl;
                return false;
            }

            SF_INFO sf_info;
            int short_mask;

            sf_info.samplerate = sampleRate;
            sf_info.channels = NumChannels;

            switch (bitDepth) {
            case 8:
                short_mask = SF_FORMAT_PCM_U8;
                break;
            case 16:
                short_mask = SF_FORMAT_PCM_16;
                break;
            case 24:
                short_mask = SF_FORMAT_PCM_24;
                break;
            case 32:
                short_mask = SF_FORMAT_PCM_32;
                break;
            default:
                short_mask = SF_FORMAT_PCM_24;
                break;
            }
            sf_info.format = SF_FORMAT_WAV | short_mask;

            if ((this->file = sf_open(path.c_str(), SFM_WRITE, &sf_info)) == NULL) {
                char errstr[256];
                sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
                std::cout << "cannot open sndfile" << path << " for output (" << errstr << ")" << std::endl;
                return false;
            }

            // enable clipping during float->int conversion
            sf_command(this->file, SFC_SET_CLIPPING, NULL, SF_TRUE);

            this->maxFrames = maxFrames;
            jack_ringbuffer_reset(this->ringBuf.get());
            dataPending = false;
            return true;
        }

        Writer()
            : SfStream(),
              dataPending(false),
              numFramesCaptured(0),
              maxFrames(JACK_MAX_FRAMES) {
        }
    }; // Writer class

    //-----------------------------------------------------------------------------------------------------------------
    //---- Reader class

    class Reader : public SfStream {
        friend class Tape;

      private:
        size_t frames{};
        size_t framesProcessed = 0;
        uint8_t inChannels = 2;
        static constexpr size_t maxFramesToRead = ringBufFrames;
        // interleaved buffer from soundfile (disk thread)
        Sample diskInBuf[frameSize * maxFramesToRead]{};
        // additional buffer for padding mono to stereo
        Sample conversionBuf[frameSize * maxFramesToRead]{};
        Sample *diskBufPtr{};
        // buffer for deinterleaving after ringbuf (audio thread)
        Sample pullBuf[frameSize * maxFramesToRead]{};
        std::atomic<bool> isPrimed{};
        bool needsData{};
        std::atomic<bool> loopFile{};

      private:
        // prime the ringbuffer
        void prime() {
            // std::cout << "priming tape reader" << std::endl;
            jack_ringbuffer_t *rb = this->ringBuf.get();
            size_t framesToRead = jack_ringbuffer_write_space(rb) / frameSize;
            if (framesToRead > maxFramesToRead) {
                framesToRead = maxFramesToRead;
            };
            auto framesRead = (size_t)sf_readf_float(this->file, diskBufPtr, framesToRead);
            if (inChannels == 1)
                convertToStereo(framesRead);
            jack_ringbuffer_write(rb, (char *)diskInBuf, frameSize * framesRead);

            // couldn't read enough, file is shorter than the buffer
            if (framesRead < framesToRead) {
                std::cout << "Tape::Reader: short file, disable loop" << std::endl;
                SfStream::shouldStop = false;
            }
        }
        void convertToStereo(size_t frameCount) {
            size_t fr = 0;
            while (fr < frameCount) {
                Sample sample = conversionBuf[fr];
                diskInBuf[2 * fr] = sample;
                diskInBuf[2 * fr + 1] = sample;
                fr++;
            }
        }

      public:
        // from audio thread
        void process(float *dst[NumChannels], size_t numFrames) {
            if (!SfStream::isRunning || !isPrimed) {
                for (size_t fr = 0; fr < numFrames; ++fr) {
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        dst[ch][fr] = 0.f;
                    }
                }
                return;
            }

            jack_ringbuffer_t *rb = this->ringBuf.get();
            auto framesInBuf = jack_ringbuffer_read_space(rb) / frameSize;

            //  if ringbuf isn't full enough, probably EOF on a non-looped file
            if (framesInBuf < numFrames) {
                // pull from ringbuffer
                jack_ringbuffer_read(rb, (char *)pullBuf, framesInBuf * frameSize);
                float *src = pullBuf;
                size_t fr = 0;
                // de-interleave, apply amp, copy to output
                while (fr < framesInBuf) {
                    float amp = SfStream::getEnvSample();
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        dst[ch][fr] = *src++ * amp;
                    }
                    fr++;
                }
                while (fr < numFrames) {
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        dst[ch][fr] = 0.f;
                    }
                    fr++;
                }
                jack_ringbuffer_reset(rb);
                SfStream::isRunning = false;
            } else {

                // pull from ringbuffer
                jack_ringbuffer_read(rb, (char *)pullBuf, numFrames * frameSize);

                if (this->mut.try_lock()) {
                    this->needsData = true;
                    this->cv.notify_one();
                    this->mut.unlock();
                }

                float *src = pullBuf;

                // de-interleave, apply amp, copy to output
                for (size_t fr = 0; fr < numFrames; ++fr) {
                    float amp = SfStream::getEnvSample();
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        dst[ch][fr] = *src++ * amp;
                    }
                }
                framesProcessed += numFrames;
            }
        }

        // from any thread
        bool open(const std::string &path) {
            if (SfStream::isRunning) {
                std::cout << "Tape Reader::open(): stream is running; no action was taken" << std::endl;
                return false;
            }

            SF_INFO sfInfo;
            if ((this->file = sf_open(path.c_str(), SFM_READ, &sfInfo)) == NULL) {
                char errstr[256];
                sf_error_str(0, errstr, sizeof(errstr) - 1);
                std::cout << "Tape Reader:: cannot open sndfile" << path << " for output (" << errstr << ")" << std::endl;
                return false;
            }

            if (sfInfo.frames < 1) {

                std::cout << "Tape Reader:: error reading file " << path << " (no frames available)" << std::endl;
                return false;
            }
            this->frames = static_cast<size_t>(sfInfo.frames);
            std::cout << "Tape Reader:: file size " << this->frames << " samples" << std::endl;
            inChannels = sfInfo.channels;
            if (inChannels > NumChannels)
                return 0; // more than stereo is going to break things
            if (inChannels == 1)
                diskBufPtr = conversionBuf; // conversion needed for mono
            else
                diskBufPtr = diskInBuf;
            framesProcessed = 0;

            jack_ringbuffer_reset(this->ringBuf.get());
            isPrimed = false;
            loopFile = true;
            if (this->frames < 48000)
                loopFile = false;
            return this->frames > 0;
        }

      private:
        // from disk thread
        void diskLoop() override {
            prime();
            isPrimed = true;
            SfStream::isRunning = true;
            SfStream::shouldStop = false;
            while (!SfStream::shouldStop) {
                {
                    std::unique_lock<std::mutex> lock(this->mut);
                    this->cv.wait(lock, [this] {
                        return this->needsData;
                    });
                    // check for spurious wakeup
                    if (!needsData) {
                        continue;
                    }
                }

                jack_ringbuffer_t *rb = this->ringBuf.get();

                size_t framesToRead = jack_ringbuffer_write_space(rb) / frameSize;
                if (framesToRead < 1) {
                    {
                        std::unique_lock<std::mutex> lock(this->mut);
                        needsData = false;
                    }
                    continue;
                }

                if (framesToRead > maxFramesToRead) {
                    // _really_ shouldn't happen
                    framesToRead = maxFramesToRead;
                };
                auto framesRead = (size_t)sf_readf_float(this->file, diskBufPtr, framesToRead);
                if (inChannels == 1)
                    convertToStereo(framesRead);
                jack_ringbuffer_write(rb, (char *)diskInBuf, frameSize * framesRead);

                if (loopFile) {
                    // couldn't perform full read so must be end of file. Seek to start of file and keep reading
                    while (framesRead < framesToRead) {
                        // std::cout << "tape reader: couldn't perform full read; looping file..." << std::endl;
                        sf_seek(this->file, 0, SEEK_SET);
                        auto nextRead = (size_t)sf_readf_float(this->file, diskBufPtr, framesToRead - framesRead);
                        if (nextRead < 1) {
                            // Shouldn't happen
                            std::cout << "Tape::Reader: unable to read file" << std::endl;
                            SfStream::shouldStop = true;
                            break;
                        }
                        if (inChannels == 1)
                            convertToStereo(framesRead);
                        jack_ringbuffer_write(rb, (char *)diskInBuf, frameSize * nextRead);
                        framesRead += nextRead;
                    }
                } else {
                    SfStream::shouldStop = true;
                }

                {
                    std::unique_lock<std::mutex> lock(this->mut);
                    needsData = false;
                }
            }
            sf_close(this->file);
        }

    }; // Reader class

    Writer writer;
    Reader reader;

  public:
    bool isWriting() {
        return writer.isRunning;
    }
    bool isReading() {
        return reader.isRunning;
    }
};

} // namespace crone

#endif // CRONE_TAPE_H
