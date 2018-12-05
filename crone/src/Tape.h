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

        public:
            SfAccess(size_t rbf = 2048) : file(nullptr), status(0), ringBufFrames(rbf) {
                ringBuf = std::unique_ptr<jack_ringbuffer_t>
                        (jack_ringbuffer_create(sampleSize * NumChannels * ringBufFrames));
            }
        };


        //--------------------------------------------------------------------------------------------------------------
        //---- Writer class

        class Writer : SfAccess {
            friend class Tape;
        protected:
            std::atomic<bool> isRunning;
        private:
            bool dataReady;
            std::atomic<bool> shouldStop;
            Sample frameBuf[frameSize];
            size_t numFramesCaptured;
            size_t maxFrames;

        public:
            // call from audio thread
            void process(const float *src[NumChannels], size_t numFrames) {
                if (!isRunning) { return; }
                // push to ringbuffer
                for (size_t fr = 0; fr < numFrames; ++fr) {
                    // libsndfile needs interleaved data, so we do that here
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        if (jack_ringbuffer_write(this->ringBuf.get(),
                                // FIXME: why does this need a C cast?
                                (const char*)(src[ch]+fr), sampleSize) < sampleSize)
                        {
                            // overrun! TODO: say something about overruns (but not here on audio thread)
                        }
                    }
                }

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
                        if (!dataReady) { continue; }
                    }
                    /// FIXME: writing tape one frame at a time! not efficient at all...
                    /// for this application we can probably assume that writes happen in [blocksize] chunks
                    while (jack_ringbuffer_read_space(this->ringBuf.get()) >= frameSize) {
                        // FIXME: why does this need a c cast?
                        jack_ringbuffer_read(this->ringBuf.get(), (char*)frameBuf, frameSize);
                        if (sf_writef_float(this->file, frameBuf, 1) != 1) {
                            char errstr[256];
                            sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
                            std::cerr << "cannot write sndfile (" << errstr << ")" << std::endl;
                            this->status = EIO;
                            break;
                        }
                        if (++numFramesCaptured >= maxFrames) {
                            break;
                        }
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
                    sf_error_str(0, errstr, sizeof(errstr) - 1);
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
            Sample frameBuf[frameSize];

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
                // push to ringbuffer
                for (size_t fr = 0; fr < numFrames; ++fr) {
                    // data is interleaved in ringbuffer
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        if (jack_ringbuffer_read_space(this->ringBuf.get()) < sampleSize) {
                            // underrun! TODO: say something about underruns (but not here on audio thread)
                        }
                        // FIXME: why is C cast needed here?
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
                if(!prime()) { return false; }
                return true;
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

                    // FIXME: again, this loop doesn't need to be per-sample.
                    /// seems like jack_ringbuffer API allows us to do a direct write to the buffer,
                    /// followed by `jack_ringbuffer_write_advance()`
                    while (jack_ringbuffer_write_space(this->ringBuf.get()) >= frameSize) {
                        sf_count_t nf = sf_readf_float(this->file, frameBuf, 1);
                        if (nf != 1) {
                            // FIXME? assuming this means we're out of stuff in the file
                            std::cerr << "Tape::Reader::diskloop() read EOF" << std::endl;
                            shouldStop = true;
                            break;
                        }
                        jack_ringbuffer_write(this->ringBuf.get(), (char*)frameBuf, frameSize);
                    }
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
