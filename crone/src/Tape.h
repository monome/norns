//
// Created by emb on 12/01/18.
//

#ifndef CRONE_TAPE_H
#define CRONE_TAPE_H

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <string>

#include <jack/types.h>
#include <jack/ringbuffer.h>
#include <sndfile.h>

#include "Window.h"

namespace crone {

    template<int NumChannels>
    class Tape {
    private:

        typedef jack_default_audio_sample_t Sample;
        static constexpr size_t sampleSize = sizeof(Sample);
        static constexpr size_t frameSize = sampleSize * NumChannels;


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
            size_t ringBufFrames;
            size_t ringBufBytes;

            typedef enum {
                Starting, Playing, Stopping, Stopped
            } EnvState;

            std::atomic<EnvState> envState;
            std::atomic<int> envIdx;

        public:
            std::atomic<bool> isRunning;
            std::atomic<bool> shouldStop;

        public:
            SfStream(size_t rbf = 2048):
            file(nullptr),
            status(0),
            ringBufFrames(rbf),
            isRunning(false),
            shouldStop(false)
            {
                ringBufBytes = sampleSize * NumChannels * ringBufFrames;
                ringBuf = std::unique_ptr<jack_ringbuffer_t>(jack_ringbuffer_create(ringBufBytes));

                envIdx = 0;
                envState = Stopped;
            }

            virtual // from any thread
            void start() {
                if (isRunning) {
                    return;
                } else {
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
                float y=0.f;
                switch (envState) {
                    case Starting:
                        y = Window::raisedCosShort[envIdx];
                        incEnv();
                        break;;
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
                    envIdx = Window::raisedCosShortLen-1;
                    envState = Playing;
                }
            }

            void decEnv() {
                envIdx--;
                if (envIdx < 0) {
                    envIdx = 0;
                    envState = Stopped;
                    shouldStop = true;
                }
            }

        };


        //--------------------------------------------------------------------------------------------------------------
        //---- Writer class

        class Writer: public SfStream {
            friend class Tape;

        private:
            static constexpr size_t maxFramesToWrite = 1024;
            bool dataReady;
            //  buffer for writing to soundfile (disk thread)
            Sample diskOutBuf[maxFramesToWrite * NumChannels];
            //  buffer for interleaving before ringbuf (audio thread)
            Sample pushBuf[maxFramesToWrite * NumChannels];
            size_t numFramesCaptured;
            size_t maxFrames;

        public:
            // call from audio thread
            void process(const float *src[NumChannels], size_t numFrames) {
                if (!SfStream::isRunning) { return; }
                // push to ringbuffer
                jack_ringbuffer_t *rb = this->ringBuf.get();
                const size_t bytesToPush = numFrames * frameSize;
                const size_t bytesAvailable = jack_ringbuffer_write_space(rb);
                if (bytesToPush > bytesAvailable) {
                    std::cerr << "Tape: writer overrun" << std::endl;
                }

                /// libsndfile requires interleaved data. we do that here before pushing to ringbuf
                float *dst = pushBuf;
                for (size_t fr = 0; fr < numFrames; ++fr) {
                    float amp = SfStream::getEnvSample();
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        *dst++ = src[ch][fr] * amp;
                    }
                }
                jack_ringbuffer_write(rb, (const char *) pushBuf, bytesToPush);

                if (this->mut.try_lock()) {
                    this->dataReady = true;
                    this->cv.notify_one();
                    this->mut.unlock();
                }
            }

            // call from disk thread
            void diskLoop() override {
                SfStream::isRunning = true;
                SfStream::shouldStop = false;
                numFramesCaptured = 0;
                while (!SfStream::shouldStop) {
                    {
                        std::unique_lock<std::mutex> lock(this->mut);
                        this->cv.wait(lock, [this] {
                            return this->dataReady;
                        });
                        // check for spurious wakeup
                        if (!dataReady) {
                            continue;
                        }
                    }

                    int framesToWrite = static_cast<int>(jack_ringbuffer_read_space(this->ringBuf.get()) / frameSize);
                    if (framesToWrite < 1) {
                        {
                            std::unique_lock<std::mutex> lock(this->mut);
                            dataReady = false;
                        }
                        continue;
                    }

                    if (framesToWrite > (int) maxFramesToWrite) { framesToWrite = (int) maxFramesToWrite; }

                    jack_ringbuffer_read(this->ringBuf.get(), (char *) diskOutBuf, framesToWrite * frameSize);
                    if (sf_writef_float(this->file, diskOutBuf, framesToWrite) != framesToWrite) {
                        char errstr[256];
                        sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
                        std::cerr << "cannot write sndfile (" << errstr << ")" << std::endl;
                        this->status = EIO;
                        break;
                    }
                    numFramesCaptured += framesToWrite;
                    if (numFramesCaptured >= maxFrames) {
                        std::cerr << "Tape: writer exceeded max frame count; aborting.";
                        break;
                    }

                }
                sf_close(this->file);
                SfStream::isRunning = false;
            }

            // from any thread
            bool open(const std::string &path,
                      size_t maxFrames = JACK_MAX_FRAMES,
                      int sampleRate = 48000,
                      int bitDepth = 24) {
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
                    std::cerr << "cannot open sndfile" << path << " for output (" << errstr << ")" << std::endl;
                    return false;
                }

                this->maxFrames = maxFrames;
                return true;
            }

            Writer() : SfStream(),
                       dataReady(false),
                       numFramesCaptured(0),
                       maxFrames(JACK_MAX_FRAMES) {}
        }; // Writer class

        //-----------------------------------------------------------------------------------------------------------------
        //---- Reader class

        class Reader : public SfStream {
            friend class Tape;
        private:
            bool needsData;
            size_t frames;
            size_t framesBeforeFadeout;
            size_t framesProcessed = 0;
            static constexpr size_t maxFramesToRead = 1024;
            // interleaved buffer from soundfile (disk thread)
            Sample diskInBuf[frameSize * maxFramesToRead];
            // buffer for deinterleaving after ringbuf (audio thread)
            Sample pullBuf[frameSize * maxFramesToRead];
            std::atomic<bool> isPrimed;

        private:
            // prime the ringbuffer
            bool prime() {
                jack_ringbuffer_t *rb = this->ringBuf.get();
                size_t framesToRead = jack_ringbuffer_write_space(rb) / frameSize;
                if (framesToRead > maxFramesToRead) { framesToRead = maxFramesToRead; };
                auto framesRead = (size_t) sf_readf_float(this->file, diskInBuf, framesToRead);
                jack_ringbuffer_write(rb, (char *) diskInBuf, frameSize * framesRead);

                if (framesRead != framesToRead) {
                    std::cerr << "Tape::Reader: warning! priming not complete" << std::endl;
                    SfStream::shouldStop = true;
                    return false;
                }

                return true;
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

                jack_ringbuffer_t* rb = this->ringBuf.get();
                auto framesInBuf = jack_ringbuffer_read_space(rb) / frameSize;

                //  if ringbuf isn't full enough, it's probably cause we're at EOF
                if(framesInBuf < numFrames) {

                    // pull from ringbuffer
                    jack_ringbuffer_read(rb, (char*)pullBuf, framesInBuf * frameSize);
                    float* src = pullBuf;
                    size_t fr = 0;
                    // de-interleave, apply amp, copy to output
                    while (fr < framesInBuf) {
                        float amp = SfStream::getEnvSample();
                        for (int ch = 0; ch < NumChannels; ++ch) {
                            dst[ch][fr] = *src++ * amp;
                        }
                        fr++;
                    }
                    while(fr < numFrames) {
                        for (int ch = 0; ch < NumChannels; ++ch) {
                            dst[ch][fr] = 0.f;
                        }
                        fr++;
                    }
                    SfStream::isRunning = false;
                } else {

                    // pull from ringbuffer
                    jack_ringbuffer_read(rb, (char *) pullBuf, numFrames * frameSize);
                    float *src = pullBuf;


                    if (framesProcessed > (framesBeforeFadeout-numFrames)) {
                        if (SfStream::envState != SfStream::EnvState::Stopping) {
                            SfStream::envState = SfStream::EnvState::Stopping;
                        }
                    }

                    // de-interleave, apply amp, copy to output
                    for (size_t fr = 0; fr < numFrames; ++fr) {
                        float amp = SfStream::getEnvSample();
                        for (int ch = 0; ch < NumChannels; ++ch) {
                            dst[ch][fr] = *src++ * amp;
                        }
                    }

                    framesProcessed += numFrames;
                }

                if (this->mut.try_lock()) {
                    this->needsData = true;
                    this->cv.notify_one();
                    this->mut.unlock();
                }

            }

            // from any thread
            bool open(const std::string &path) {
                SF_INFO sfInfo;

                if ((this->file = sf_open(path.c_str(), SFM_READ, &sfInfo)) == NULL) {
                    char errstr[256];
                    sf_error_str(0, errstr, sizeof(errstr) - 1);
                    std::cerr << "cannot open sndfile" << path << " for output (" << errstr << ")" << std::endl;
                    return false;
                }

                if (sfInfo.frames < 1) {

                    std::cerr << "error reading file " << path << " (no frames available)" << std::endl;
                    return false;
                }

                this->frames = static_cast<size_t>(sfInfo.frames);
                framesBeforeFadeout = this->frames - Window::raisedCosShortLen - 1;
                framesProcessed = 0;

                jack_ringbuffer_reset(this->ringBuf.get());
                isPrimed = false;

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
                        if (!needsData) { continue; }
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

                    if (framesToRead > maxFramesToRead) { framesToRead = maxFramesToRead; };
                    auto framesRead = (size_t) sf_readf_float(this->file, diskInBuf, framesToRead);
                    if (framesRead < framesToRead) {
                        std::cerr << "Tape::Reader::diskloop() read EOF" << std::endl;
                        SfStream::shouldStop = true;
                    }

                    jack_ringbuffer_write(rb, (char *) diskInBuf, frameSize * framesRead);

                }
                sf_close(this->file);
            }

        }; // Reader class

        Writer writer;
        Reader reader;

    public:
        bool isWriting() { return writer.isRunning; }
        bool isReading() { return reader.isRunning; }
    };

}

#endif // CRONE_TAPE_H
