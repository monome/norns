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

        class SfAccess {
        protected:
            SNDFILE *file;
            std::unique_ptr<std::thread> th;
            std::mutex mut;
            std::condition_variable cv;
            std::unique_ptr<jack_ringbuffer_t> ringBuf;
            volatile int status;
            size_t ringBufFrames;
            size_t ringBufBytes;

        public:
            SfAccess(size_t rbf = 2048) : file(nullptr), status(0), ringBufFrames(rbf) {
                ringBufBytes = sampleSize * NumChannels * ringBufFrames;
                ringBuf = std::unique_ptr<jack_ringbuffer_t>(jack_ringbuffer_create(ringBufBytes));
            }
        };


        //--------------------------------------------------------------------------------------------------------------
        //---- Writer class

        class Writer : SfAccess {
            friend class Tape;
        protected:
            std::atomic<bool> isRunning;
        private:
            static constexpr size_t maxFramesToWrite = 1024;
            bool dataReady;
            std::atomic<bool> shouldStop;
            // additional buffer for writing to soundfile
            Sample diskOutBuf[maxFramesToWrite * NumChannels];
            // additional buffer for interleaving before pushing to ring buffer
            Sample pushOutBuf[maxFramesToWrite * NumChannels];
            size_t numFramesCaptured;
            size_t maxFrames;

        public:
            // call from audio thread
            void process(const float *src[NumChannels], size_t numFrames) {
                if (!isRunning) { return; }
                // push to ringbuffer
                jack_ringbuffer_t* rb = this->ringBuf.get();
                const size_t bytesToPush = numFrames * frameSize;
                const size_t bytesAvailable = jack_ringbuffer_write_space(rb);
                if(bytesToPush > bytesAvailable) {
                    std::cerr << "Tape: writer overrun" << std::endl;
                }

                /// libsndfile requires interleaved data. we do that here before pushing to ringbuf
                /// TODO: this is where we would apply an amplitude envelope
                float* dst = pushOutBuf;
                for (size_t fr = 0; fr < numFrames; ++fr) {
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        *dst++ = src[ch][fr];
                    }
                }
                jack_ringbuffer_write(rb, (const char*)pushOutBuf, bytesToPush);
#if 0
                // dummy check
                size_t framesInRingBuf = jack_ringbuffer_read_space(this->ringBuf.get()) / frameSize;
                std::cerr << "frames in ringbuf on audio thread: " << framesInRingBuf << std::endl;
#endif
                if (this->mut.try_lock()) {
                    this->dataReady = true;
                    this->cv.notify_one();
                    this->mut.unlock();
                }
            }

            // call from disk thread
            void diskLoop() {
                isRunning = true;
                numFramesCaptured = 0;
                shouldStop = false;
                while (!shouldStop) {

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
#if 0
                    // dummy check
                    std::cerr << "frames in ringbuf on disk thread: " << framesToWrite << std::endl;
#endif
                    if (framesToWrite > (int)maxFramesToWrite) { framesToWrite = (int)maxFramesToWrite; }

                    jack_ringbuffer_read(this->ringBuf.get(), (char*)diskOutBuf, framesToWrite * frameSize);
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
                isRunning = false;
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

            // from any thread
            void start() {
                if (isRunning) {
                    // TODO: tape capture is already running; what to do...
                    /// - could forcibly terminate old thread and start a new one,
                    ///   but then need to handle file cleanup (class wrapper with d-tor?)
                    /// - probably better to issue stop and wait/join on disk thread.
                    ///   but that means stalling the caller (which in this application is likely fine?)
                    /// - for now, just ignoring start is probably ok i think
                } else {
                    this->th = std::make_unique<std::thread>(
                            [this]() {
                                this->diskLoop();
                            });
                    this->th->detach();
                }
            }

            // from any thread
            void stop() {
                shouldStop = true;
            }

            Writer() : SfAccess(),
                       isRunning(false),
                       dataReady(false),
                       shouldStop(false),
                       numFramesCaptured(0),
                       maxFrames(JACK_MAX_FRAMES) {}
        };

    //-----------------------------------------------------------------------------------------------------------------
    //---- Reader class

        class Reader : SfAccess {
            friend class Tape;
        protected:
            std::atomic<bool> isRunning;
        private:
            std::atomic<bool> shouldStop;
            bool needsData;
            size_t frames;
            static constexpr size_t maxFramesToRead= 1024;
            Sample frameBuf[frameSize * maxFramesToRead];

        private:
            // prime the ringbuffer
            bool prime() {
                sf_count_t nf;
                for (size_t fr = 0; fr < this->ringBufFrames; ++fr) {
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        // FIXME: ? using "frames" variant, i dunno
                        nf = sf_readf_float(this->file, frameBuf, 1);
                        if (nf != 1) {
                            std::cerr << "error priming ringbuffer; couldn't read frame " << fr << std::endl;
                            return false;
                        }
                        if (jack_ringbuffer_write_space(this->ringBuf.get()) < frameSize) {
                            // double-check, shouldn't really get here
                            break;
                        }
                        jack_ringbuffer_write(this->ringBuf.get(), (char *) frameBuf, frameSize);
                    }
                }
                return true;
            }
        public:
            // from audio thread
            void process(float *dst[NumChannels], size_t numFrames) {
                if (!isRunning) { return; }
                // pull from ringbuffer
                for (size_t fr = 0; fr < numFrames; ++fr) {
                    // data is interleaved in ringbuffer
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        // FIXME: not sure what jack ringbuf overhead is,
                        // but probably faster to read `numFrames` into an intermediate buffer, then de-interleave
                        if (jack_ringbuffer_read_space(this->ringBuf.get()) < sampleSize) {
                            // underrun! TODO: say something about underruns (but not here on audio thread)
                        }
                        // FIXME: why is C cast needed here?
                        // TODO: add amp envelope
                        jack_ringbuffer_read(this->ringBuf.get(), (char*)(dst[ch]+fr), sampleSize);
                    }
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

                if(sfInfo.frames < 1) {

                    std::cerr << "error reading file " << path << " (no frames available)" << std::endl;
                    return false;
                }

                this->frames = static_cast<size_t>(sfInfo.frames);
                if (prime()) return true;
                return false;
            }

            // from any thread
            void start() {

                if (isRunning) {
                    return;
                } else {
                    this->th = std::make_unique<std::thread>(
                            [this]() {
                                this->diskLoop();
                            });
                    this->th->detach();
                }
            }

            // from any thread
            void stop() {
                shouldStop = true;
            }



        private:
            // from disk thread
            void diskLoop() {
                prime();
                isRunning = true;
                shouldStop = false;
                while (!shouldStop) {
                    {
                        std::unique_lock<std::mutex> lock(this->mut);
                        this->cv.wait(lock, [this] {
                            return this->needsData;
                        });
                        // check for spurious wakeup
                        if (!needsData) { continue; }
                    }

                    jack_ringbuffer_t* rb = this->ringBuf.get();

                    size_t framesToRead = jack_ringbuffer_write_space(rb) / frameSize;
                    if (framesToRead < 1) {
                        {
                            std::unique_lock<std::mutex> lock(this->mut);
                            needsData  = false;
                        }
                        continue;
                    }

                    if (framesToRead > maxFramesToRead) { framesToRead = maxFramesToRead; };
                    auto framesRead = (size_t) sf_readf_float(this->file, frameBuf, framesToRead);
                    if (framesRead != framesToRead) {
                        std::cerr << "Tape::Reader::diskloop() read EOF" << std::endl;
                        shouldStop = true;
                    }

                    jack_ringbuffer_write(rb, (char*)frameBuf, frameSize * framesRead);

                }
                sf_close(this->file);
                isRunning = false;
            }

        };

        Writer writer;
        Reader reader;

    public:
        bool isWriting() { return writer.isRunning; }
        bool isReading() { return reader.isRunning; }

    };

}

#endif // CRONE_TAPE_H
