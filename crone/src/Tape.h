//
// Created by emb on 12/01/18.
//

#ifndef CRONE_TAPE_H
#define CRONE_TAPE_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <cerrno>
#include <cstring>
#include <jack/ringbuffer.h>
#include <jack/types.h>
#include <sndfile.h>

#include "Window.h"
#include "readerwriterqueue.h"

namespace crone {

template <int NumChannels>
class Tape {
  public:
    enum class TransportState {
        Idle,
        Starting,
        Running,
        Pausing,
        Paused,
        Stopping,
        Stopped
    };

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
        // protects disk thread coordination via condition variable
        std::mutex diskMutex;
        // guards lifecycle operations (open/start/stop/join) against races
        std::mutex lifecycleMutex;
        // prevents multiple threads from adding commands to the queue at once
        std::mutex cmdMutex;
        // signals disk thread for data availability and shutdown coordination
        std::condition_variable cv;

        std::atomic<int> status; // libsndfile error code - unused at present

        // fade envelope state
        enum class EnvState {
            FadeIn,
            On,
            FadeOut,
            Off
        };

        std::atomic<EnvState> envState;
        std::atomic<int> envIdx;
        std::atomic<TransportState> transportState;

        // incremented on each start(); all enqueued commands are stamped with
        // the current epoch and the consumer drops stale ones from prior runs
        std::atomic<uint32_t> runEpoch{0};

        // tape command types for centralized state management
        enum class Command { Start,
                             Pause,
                             Resume,
                             Stop,
                             SetLoop };

        // command structure for command queue
        // - arg is command-specific (e.g. SetLoop uses arg as bool)
        // - epoch is used as a run id for discarding stale commands
        struct Cmd {
            Command type;
            uint32_t arg;
            uint32_t epoch;
        };
        static constexpr size_t cmdQueueCapacity = 64;
        moodycamel::ReaderWriterQueue<Cmd> cmdQueue{cmdQueueCapacity};

        // ringbuffer for providing data between disk and audio threads (lock-free, single producer single consumer)
        // - jack_ringbuffer_t must be freed with jack_ringbuffer_free
        // - use unique_ptr with custom deleter to ensure proper cleanup
        using RingbufPtr = std::unique_ptr<jack_ringbuffer_t, decltype(&jack_ringbuffer_free)>;
        RingbufPtr ringBuf{nullptr, &jack_ringbuffer_free};

        // envelope completion signals for state transitions
        std::atomic<bool> fadeInDone{false};
        std::atomic<bool> fadeOutDone{false};

      public:
        std::atomic<bool> isRunning;
        std::atomic<bool> shouldStop;

      public:
        SfStream()
            : file(nullptr),
              status(0),
              ringBuf(jack_ringbuffer_create(ringBufBytes), &jack_ringbuffer_free),
              isRunning(false),
              shouldStop(false) {
            envIdx = 0;
            envState = EnvState::Off;
            transportState = TransportState::Idle;
        }

        virtual ~SfStream() {
            std::lock_guard<std::mutex> lock(lifecycleMutex);
            if (th && th->joinable()) {
                shouldStop = true;
                cv.notify_one();
                th->join();
            }
        }

        virtual void start() {
            std::lock_guard<std::mutex> lock(lifecycleMutex);
            if (isRunning) {
                std::cout << "Tape::SfStream::start(): already running" << std::endl;
                return;
            }

            // ensure any previous thread is cleaned up
            if (th && th->joinable()) {
                shouldStop = true;
                cv.notify_one();
                th->join();
            }

            if (file == nullptr) {
                std::cout << "Tape::SfStream::start(): no file open; aborting start" << std::endl;
                return;
            }

            std::cout << "Tape::SfStream: starting..." << std::endl;
            shouldStop = false;

            // bump command epoch so any pre-existing commands are ignored by consumer
            runEpoch.fetch_add(1, std::memory_order_acq_rel);

            th = std::make_unique<std::thread>(
                [this]() {
                    diskLoop();
                });

            bool ok = enqueueCmd(Command::Start);
            if (!ok) {
                std::cout << "Tape::SfStream::start(): command queue full, dropping Start" << std::endl;
            }
        }

        void pause() {
            bool ok = enqueueCmd(Command::Pause);
            if (!ok) {
                std::cout << "Tape::SfStream::pause(): command queue full, dropping Pause" << std::endl;
            }
        }

        void resume() {
            bool ok = enqueueCmd(Command::Resume);
            if (!ok) {
                std::cout << "Tape::SfStream::resume(): command queue full, dropping Resume" << std::endl;
            }
        }

        void stop() {
            // non-blocking stop: request fade-out/stop and return.
            // cleanup (thread join) happens when the disk thread exits after the audio thread transitions to Stopped.
            bool ok = enqueueCmd(Command::Stop);
            if (!ok) {
                std::cout << "Tape::SfStream::stop(): command queue full, dropping Stop" << std::endl;
            }
        }

        // from any thread
        bool isPaused() const noexcept {
            return transportState == TransportState::Paused;
        }

      protected:
        // derived classes implement disk i/o loop
        virtual void diskLoop() = 0;

        // process derived-class specific commands on audio thread
        // must be real-time-safe: no locks, no i/o, atomics only
        virtual void processDerivedCommand(const Cmd &) noexcept {
        }

        // safely adds a command to the queue with a timestamp to track which session it belongs to
        bool enqueueCmd(Command type, uint32_t arg = 0u) {
            std::lock_guard<std::mutex> lock(cmdMutex);
            Cmd c{type, arg, runEpoch.load(std::memory_order_relaxed)};
            return cmdQueue.try_enqueue(c);
        }

        // process pending commands on audio thread - centralized transport state management
        void processCommands() {
            Cmd c;
            while (cmdQueue.try_dequeue(c)) {
                // discard commands issued prior to the most recent start()
                if (c.epoch != runEpoch.load(std::memory_order_relaxed)) {
                    continue;
                }
                switch (c.type) {
                case Command::Start:
                    if (transportState != TransportState::Running &&
                        transportState != TransportState::Starting) {
                        envIdx = 0;
                        envState = EnvState::FadeIn;
                        transportState = TransportState::Starting;
                    }
                    break;

                case Command::Pause:
                    if (transportState == TransportState::Running ||
                        transportState == TransportState::Starting) {
                        envState = EnvState::FadeOut;
                        transportState = TransportState::Pausing;
                    }
                    break;

                case Command::Resume:
                    if (transportState == TransportState::Paused) {
                        envIdx = 0;
                        envState = EnvState::FadeIn;
                        transportState = TransportState::Starting;
                    }
                    break;

                case Command::Stop:
                    envState = EnvState::FadeOut;
                    transportState = TransportState::Stopping;
                    break;

                case Command::SetLoop:
                    processDerivedCommand(c);
                    break;
                }
            }

            // handle envelope-driven state transitions
            if (fadeInDone.exchange(false)) {
                if (transportState == TransportState::Starting) {
                    transportState = TransportState::Running;
                }
            }
            if (fadeOutDone.exchange(false)) {
                if (transportState == TransportState::Pausing) {
                    transportState = TransportState::Paused;
                } else if (transportState == TransportState::Stopping) {
                    transportState = TransportState::Stopped;
                    shouldStop = true; // signal disk thread to exit
                    // ensure disk thread wakes if it is waiting on CV (safe to notify without holding a lock)
                    cv.notify_one();
                }
            }
        }

        float getEnvSample() {
            float y = 0.f;
            EnvState currentEnvState = envState;
            switch (currentEnvState) {
            case EnvState::FadeIn:
                y = Window::raisedCosShort[envIdx];
                incEnv();
                break;
            case EnvState::FadeOut:
                y = Window::raisedCosShort[envIdx];
                decEnv();
                break;
            case EnvState::On:
                y = 1.0f;
                break;
            case EnvState::Off:
            default:
                y = 0.f;
                break;
            }
            return y;
        }

      private:
        void incEnv() {
            int currentIdx = envIdx.fetch_add(1);
            if (currentIdx >= static_cast<int>(Window::raisedCosShortLen) - 1) {
                envIdx = static_cast<int>(Window::raisedCosShortLen) - 1;
                envState = EnvState::On;
                fadeInDone = true;
            }
        }

        void decEnv() {
            int currentIdx = envIdx.fetch_sub(1);
            if (currentIdx <= 0) {
                envIdx = 0;
                envState = EnvState::Off;
                fadeOutDone = true;
            }
        }
    };

    //--------------------------------------------------------------------------------------------------------------
    //---- Writer class

    class Writer : public SfStream {
        friend class Tape;

      private:
        static constexpr size_t maxFramesToWrite = 4096;
        static constexpr size_t minBytesToWrite = 2048; // totally arbitrary
        bool dataPending;
        //  buffer for writing to soundfile (disk thread)
        Sample diskOutBuf[maxFramesToWrite * NumChannels];
        //  buffer for interleaving before ringbuf (audio thread)
        Sample pushBuf[maxFramesToWrite * NumChannels];
        std::atomic<size_t> numFramesCaptured;
        std::atomic<size_t> maxFrames; // recording limit

      protected:
      public:
        // stop recording if running… otherwise, close any armed file and reset state
        void stop() {
            if (SfStream::isRunning) {
                SfStream::stop();
                return;
            }
            std::lock_guard<std::mutex> lifecycleLock(this->lifecycleMutex);
            if (this->file != nullptr) {
                sf_close(this->file);
                this->file = nullptr;
            }
            jack_ringbuffer_reset(this->ringBuf.get());
            this->dataPending = false;
            numFramesCaptured = 0;
            this->envIdx = 0;
            this->envState = SfStream::EnvState::Off;
            this->transportState = TransportState::Idle;
        }

        // call from audio thread
        // audio thread: record input to disk via ringbuffer
        void process(const float *src[NumChannels], size_t numFrames) {
            this->processCommands(); // handle state changes first

            if (!SfStream::isRunning) {
                return;
            }

            // when paused, skip audio capture entirely (record-pause behavior)
            if (SfStream::transportState == TransportState::Paused) {
                return;
            }

            // defensive clamp to local buffer capacity
            if (numFrames > maxFramesToWrite) {
                numFrames = maxFramesToWrite;
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

            // libsndfile requires interleaved data. we do that here before pushing to ringbuf
            float *dst = pushBuf;
            for (size_t fr = 0; fr < numFrames; ++fr) {
                float amp = SfStream::getEnvSample();
                for (int ch = 0; ch < NumChannels; ++ch) {
                    *dst++ = src[ch][fr] * amp;
                }
            }
            jack_ringbuffer_write(rb, (const char *)pushBuf, bytesToPush);

            {
                std::lock_guard<std::mutex> lock(this->diskMutex);
                this->dataPending = true;
                this->cv.notify_one();
            }
        }

        // call from disk thread
        // write audio data to file
        void diskLoop() override {
            SfStream::isRunning = true;
            SfStream::shouldStop = false;
            numFramesCaptured = 0;
            size_t bytesAvailable;
            while (!SfStream::shouldStop) {
                {
                    std::unique_lock<std::mutex> lock(this->diskMutex);
                    this->cv.wait(lock, [this] {
                        return this->dataPending || SfStream::shouldStop;
                    });

                    // check for spurious wakeup
                    if (!dataPending) {
                        if (SfStream::shouldStop) {
                            break;
                        }
                        continue;
                    }
                }

                bytesAvailable = jack_ringbuffer_read_space(this->ringBuf.get());
                if (bytesAvailable < minBytesToWrite) {
                    {
                        std::unique_lock<std::mutex> lock(this->diskMutex);
                        this->dataPending = false;
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
                    std::unique_lock<std::mutex> lock(this->diskMutex);
                    this->dataPending = false;
                }

                if (sf_writef_float(this->file, diskOutBuf, framesToWrite) != framesToWrite) {
                    std::cout << "error: Tape::writer failed to write (libsndfile: " << sf_strerror(this->file) << ")" << std::endl;
                    this->status = EIO;
                    SfStream::shouldStop = true;
                    break;
                }

                size_t captured = numFramesCaptured.fetch_add(framesToWrite) + framesToWrite;
                if (captured >= maxFrames.load()) {
                    std::cout << "Tape: writer exceeded max frame count; aborting.";
                    break; // stop recording when limit reached
                }
            }

            // prevent further audio-thread pushes while we flush remaining data
            SfStream::isRunning = false;
            // flush any remaining frames in the ringbuffer regardless of size
            jack_ringbuffer_t *rb = this->ringBuf.get();
            while (jack_ringbuffer_read_space(rb) >= frameSize) {
                int framesToWrite = (int)(jack_ringbuffer_read_space(rb) / frameSize);
                if (framesToWrite > (int)maxFramesToWrite) {
                    framesToWrite = (int)maxFramesToWrite;
                }
                jack_ringbuffer_read(rb, (char *)diskOutBuf, framesToWrite * frameSize);
                if (sf_writef_float(this->file, diskOutBuf, framesToWrite) != framesToWrite) {
                    std::cout << "error: Tape::writer flush failed (libsndfile: " << sf_strerror(this->file) << ")" << std::endl;
                    this->status = EIO;
                    break;
                }
                numFramesCaptured.fetch_add(framesToWrite);
            }

            std::cout << "Tape::writer closing file...";
            sf_close(this->file);
            this->file = nullptr;
            std::cout << " done." << std::endl;
        }

        // open file for writing - from any thread
        bool open(const std::string &path,
                  size_t maxFrames = JACK_MAX_FRAMES, // <-- ridiculous big number
                  int sampleRate = 48000,
                  int bitDepth = 24) {

            // serialize against disk thread lifecycle and ensure prior thread is fully cleaned up
            std::lock_guard<std::mutex> lifecycleLock(this->lifecycleMutex);
            if (SfStream::isRunning) {
                std::cout << "Tape Writer::open(): stream is running; no action was taken" << std::endl;
                return false;
            }
            if (this->th && this->th->joinable()) {
                this->shouldStop = true;
                this->cv.notify_one();
                this->th->join();
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

            if ((this->file = sf_open(path.c_str(), SFM_WRITE, &sf_info)) == nullptr) {
                std::cout << "cannot open sndfile" << path << " for output (" << sf_strerror(nullptr) << ")" << std::endl;
                return false;
            }

            // reset captured frame counter so READY state reflects 0 position
            numFramesCaptured = 0;

            // enable clipping during float->int conversion
            sf_command(this->file, SFC_SET_CLIPPING, nullptr, SF_TRUE);

            this->maxFrames = maxFrames;
            jack_ringbuffer_reset(this->ringBuf.get());
            this->dataPending = false;

            this->envIdx = 0;
            this->envState = SfStream::EnvState::Off;
            this->transportState = TransportState::Idle;

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
        std::atomic<size_t> frames{0};
        std::atomic<size_t> framesProcessed{0};
        uint8_t inChannels = 2;
        std::atomic<float> fileSampleRate{48000.f};
        static constexpr size_t minFramesForLoop = 48000;
        static constexpr size_t maxFramesToRead = 4096;
        // interleaved buffer from soundfile (disk thread)
        Sample diskInBuf[NumChannels * maxFramesToRead]{};
        // additional buffer for padding mono to stereo
        Sample conversionBuf[NumChannels * maxFramesToRead]{};
        Sample *diskBufPtr{};
        // buffer for deinterleaving after ringbuf (audio thread)
        Sample pullBuf[NumChannels * maxFramesToRead]{};
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
            size_t framesRead = (size_t)sf_readf_float(this->file, diskBufPtr, framesToRead);

            // handle looping: if file is set to loop and we didn't read enough frames,
            // seek back to start and read remaining frames to fill the buffer
            if (loopFile && framesRead < framesToRead) {
                // seek back to beginning of file
                if (sf_seek(this->file, 0, SEEK_SET) == -1) {
                    this->status = EIO;
                    SfStream::shouldStop = true;
                } else {
                    // read remaining frames starting after what we already read
                    float *buffer_offset = diskBufPtr + (framesRead * inChannels);
                    size_t nextRead = (size_t)sf_readf_float(this->file, buffer_offset, framesToRead - framesRead);
                    if (nextRead > 0) {
                        framesRead += nextRead;
                    } else {
                        // handle read error or end of file
                        if (sf_error(this->file) != SF_ERR_NO_ERROR) {
                            this->status = EIO;
                        }
                        SfStream::shouldStop = true;
                    }
                }
            }

            if (inChannels == 1)
                convertToStereo(framesRead);
            jack_ringbuffer_write(rb, (char *)diskInBuf, frameSize * framesRead);
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
        void setLooping(bool loop) {
            // update immediately so status polls reflect state even when idle
            loopFile = loop;
            bool ok = this->enqueueCmd(SfStream::Command::SetLoop, loop ? 1u : 0u);
            if (!ok) {
                std::cout << "Tape::Reader::setLooping(): command queue full, dropping SetLoop" << std::endl;
            }
        }

      protected:
        void processDerivedCommand(const typename SfStream::Cmd &c) noexcept override {
            if (c.type == SfStream::Command::SetLoop) {
                loopFile = (c.arg != 0);
            }
        }

      public:
        // from audio thread
        // audio thread: read from disk via ringbuffer to output
        void process(float *dst[NumChannels], size_t numFrames) {
            this->processCommands(); // handle state changes first

            // allow draining of ringbuffer even after disk thread has stopped
            if (!isPrimed) {
                for (size_t fr = 0; fr < numFrames; ++fr) {
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        dst[ch][fr] = 0.f;
                    }
                }
                return;
            }

            // when paused, output silence without consuming ringbuffer
            if (SfStream::transportState == TransportState::Paused) {
                for (size_t fr = 0; fr < numFrames; ++fr) {
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        dst[ch][fr] = 0.f;
                    }
                }
                return;
            }

            jack_ringbuffer_t *rb = this->ringBuf.get();
            auto framesInBuf = jack_ringbuffer_read_space(rb) / frameSize;

            // handle underrun: not enough data in ringbuffer
            if (framesInBuf < numFrames) {
                // pull from ringbuffer
                jack_ringbuffer_read(rb, (char *)pullBuf, framesInBuf * frameSize);
                float *src = pullBuf;
                size_t fr = 0;
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
                // account for frames actually consumed when computing position
                framesProcessed.fetch_add(framesInBuf);
                // request more data from disk thread; disk thread will signal stop on EOF for non-loop
                {
                    std::lock_guard<std::mutex> lock(this->diskMutex);
                    this->needsData = true;
                    this->cv.notify_one();
                }
            } else {

                // pull from ringbuffer (defensively cap to local buffer capacity)
                size_t framesToPull = numFrames;
                if (framesToPull > maxFramesToRead) {
                    framesToPull = maxFramesToRead;
                }
                jack_ringbuffer_read(rb, (char *)pullBuf, framesToPull * frameSize);

                {
                    std::lock_guard<std::mutex> lock(this->diskMutex);
                    this->needsData = true;
                    this->cv.notify_one();
                }

                float *src = pullBuf;

                for (size_t fr = 0; fr < framesToPull; ++fr) {
                    float amp = SfStream::getEnvSample();
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        dst[ch][fr] = *src++ * amp;
                    }
                }
                // if the host buffer is larger than our pull buffer, pad the remainder with silence.
                // this mirrors the underrun behavior (when framesInBuf < numFrames), keeping output
                // deterministic and avoiding uninitialized tail data.
                for (size_t fr = framesToPull; fr < numFrames; ++fr) {
                    for (int ch = 0; ch < NumChannels; ++ch) {
                        dst[ch][fr] = 0.f;
                    }
                }
                framesProcessed.fetch_add(framesToPull);
            }
            // if disk thread has stopped and the ringbuffer is drained, mark stream unprimed
            if (!SfStream::isRunning) {
                if (jack_ringbuffer_read_space(this->ringBuf.get()) == 0) {
                    // ringbuffer fully drained after EOF: park reader as unprimed
                    isPrimed = false;
                    // reset position so UI shows 0:00 when READY
                    framesProcessed.store(0, std::memory_order_relaxed);
                }
            }
        }

        // open file for reading - from any thread
        bool open(const std::string &path) {
            std::lock_guard<std::mutex> lifecycleLock(this->lifecycleMutex);
            if (SfStream::isRunning) {
                std::cout << "Tape Reader::open(): stream is running; no action was taken" << std::endl;
                return false;
            }
            if (this->th && this->th->joinable()) {
                this->shouldStop = true;
                this->cv.notify_one();
                this->th->join();
            }

            SF_INFO sfInfo;
            if ((this->file = sf_open(path.c_str(), SFM_READ, &sfInfo)) == nullptr) {
                std::cout << "Tape Reader:: cannot open sndfile " << path << " for input (" << sf_strerror(nullptr) << ")" << std::endl;
                return false;
            }

            if (sfInfo.frames < 1) {
                std::cout << "Tape Reader:: error reading file " << path << " (no frames available)" << std::endl;
                sf_close(this->file);
                this->file = nullptr;
                return false;
            }
            frames = static_cast<size_t>(sfInfo.frames);
            fileSampleRate = static_cast<float>(sfInfo.samplerate);
            std::cout << "Tape Reader:: file size " << frames << " samples" << std::endl;
            inChannels = sfInfo.channels;
            if (inChannels > NumChannels) {
                sf_close(this->file);
                this->file = nullptr;
                return false;
            }
            if (inChannels == 1)
                diskBufPtr = conversionBuf;
            else
                diskBufPtr = diskInBuf;
            framesProcessed = 0;

            jack_ringbuffer_reset(this->ringBuf.get());
            isPrimed = false;

            // default to looping unless file is too short
            loopFile = true;
            if (frames < minFramesForLoop)
                loopFile = false;

            this->envIdx = 0;
            this->envState = SfStream::EnvState::Off;
            this->transportState = TransportState::Idle;

            return frames > 0;
        }

        bool getLoopFile() const {
            return loopFile;
        }

        void start() override {
            isPrimed = false;
            {
                std::unique_lock<std::mutex> lock(this->diskMutex);
                needsData = false;
            }
            this->transportState = TransportState::Idle;
            framesProcessed = 0;
            jack_ringbuffer_reset(this->ringBuf.get());
            SfStream::start();
        }

      private:
        // disk thread: read audio file and feed ringbuffer
        void diskLoop() override {
            needsData = false;
            prime(); // pre-fill ringbuffer
            isPrimed = true;
            SfStream::isRunning = true;
            SfStream::shouldStop = false;
            while (!SfStream::shouldStop) {
                {
                    std::unique_lock<std::mutex> lock(this->diskMutex);
                    this->cv.wait(lock, [this] {
                        return this->needsData || SfStream::shouldStop;
                    });
                    if (!needsData) {
                        if (SfStream::shouldStop) {
                            break;
                        }
                        continue;
                    }
                }

                jack_ringbuffer_t *rb = this->ringBuf.get();

                size_t framesToRead = jack_ringbuffer_write_space(rb) / frameSize;
                if (framesToRead < 1) {
                    {
                        std::unique_lock<std::mutex> lock(this->diskMutex);
                        this->needsData = false;
                    }
                    continue;
                }

                if (framesToRead > maxFramesToRead) {
                    framesToRead = maxFramesToRead;
                };
                size_t framesRead = (size_t)sf_readf_float(this->file, diskBufPtr, framesToRead);

                if (loopFile) {
                    // wrap to file start when not enough data read
                    if (framesRead < framesToRead) {
                        // std::cout << "tape reader: couldn't perform full read; looping file..." << std::endl;
                        if (sf_seek(this->file, 0, SEEK_SET) == -1) {
                            this->status = EIO;
                            SfStream::shouldStop = true;
                            continue;
                        }
                        float *buffer_offset = diskBufPtr + (framesRead * inChannels);
                        auto nextRead = (size_t)sf_readf_float(this->file, buffer_offset, framesToRead - framesRead);
                        if (nextRead < 1) {
                            // file read failed after loop wrap
                            if (sf_error(this->file) != SF_ERR_NO_ERROR) {
                                this->status = EIO;
                            }
                            std::cout << "Tape::Reader: unable to read file" << std::endl;
                            SfStream::shouldStop = true;
                        } else {
                            framesRead += nextRead;
                        }
                    }
                } else {
                    if (framesRead < framesToRead) {
                        if (sf_error(this->file) != SF_ERR_NO_ERROR) {
                            this->status = EIO;
                        }
                        // mark stop; tail will decide park vs close
                        SfStream::shouldStop = true;
                    }
                }

                if (framesRead > 0) {
                    if (inChannels == 1)
                        convertToStereo(framesRead);
                    jack_ringbuffer_write(rb, (char *)diskInBuf, frameSize * framesRead);
                }

                {
                    std::unique_lock<std::mutex> lock(this->diskMutex);
                    this->needsData = false;
                }
            }
            // on stop, either park (keep file open, rewind) or close file
            bool explicitStop = (SfStream::transportState == TransportState::Stopping);
            if (!explicitStop && this->status == 0 && !loopFile) {
                // rewind to start and clear buffers; remain ready with file loaded
                if (sf_seek(this->file, 0, SEEK_SET) == -1) {
                    this->status = EIO;
                    sf_close(this->file);
                    this->file = nullptr;
                    isPrimed = false;
                }
            } else {
                // fully close file and reset buffers
                sf_close(this->file);
                this->file = nullptr;
                jack_ringbuffer_reset(this->ringBuf.get());
                isPrimed = false;
            }
            SfStream::isRunning = false;
        }

    }; // Reader class

    Writer writer;
    Reader reader;
    std::atomic<float> systemSampleRate{48000.f};

  public:
    bool isWriting() const noexcept {
        return writer.isRunning.load(std::memory_order_relaxed);
    }
    bool isReading() const noexcept {
        return reader.isRunning.load(std::memory_order_relaxed);
    }
    // true while ringbuffer is primed with audio for playback
    bool playbackIsPrimed() const noexcept {
        return reader.isPrimed.load(std::memory_order_relaxed);
    }
    bool isLooping() const noexcept {
        return reader.getLoopFile();
    }
    void setLooping(bool loop) {
        reader.setLooping(loop);
    }
    void pauseRecord(bool paused) {
        if (isWriting()) {
            if (paused) {
                writer.pause();
            } else {
                writer.resume();
            }
        }
    }
    void pausePlayback(bool paused) {
        if (isReading()) {
            if (paused) {
                reader.pause();
            } else {
                reader.resume();
            }
        }
    }

    void setSampleRate(float sr) {
        systemSampleRate = sr;
    }
    float getSampleRate() const {
        return systemSampleRate;
    }
    bool playbackHasFile() const {
        return reader.file != nullptr;
    }
    bool recordHasFile() const {
        return writer.file != nullptr;
    }
    bool playbackIsPaused() const {
        return reader.isPaused();
    }
    bool recordIsPaused() const {
        return writer.isPaused();
    }
    size_t playbackFramesProcessed() const {
        return reader.framesProcessed.load(std::memory_order_relaxed);
    }
    size_t playbackFramesTotal() const {
        return reader.frames.load(std::memory_order_relaxed);
    }
    float playbackFileSampleRate() const {
        return reader.fileSampleRate.load(std::memory_order_relaxed);
    }
    size_t recordFramesCaptured() const {
        return writer.numFramesCaptured.load(std::memory_order_relaxed);
    }
};

} // namespace crone

#endif // CRONE_TAPE_H
