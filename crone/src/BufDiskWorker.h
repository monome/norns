//
// Created by emb on 11/30/19.
//
/*
 * BufDiskWorker is an Audio Buffer Disk Interface static class.
 *
 * it requires users to _register_ buffers (returns numerical index for registered buf)
 * disk read/write work can be requested for registered buffers, executed in background thread
 *
 * TODO: 
 *  - callback for request completion?
 *  - use condvar for signaling, instead of sleep+poll
 */

#ifndef CRONE_BUFMANAGER_H
#define CRONE_BUFMANAGER_H

#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <memory>

namespace crone {

    // class for asynchronous management of mono audio buffers
    class BufDiskWorker {

        enum class JobType {
            Clear,
            ReadMono, ReadStereo,
            WriteMono, WriteStereo
        };
        struct Job {
            JobType type;
            size_t bufIdx[2];
            std::string path;
            float startSrc;
            float startDst;
            float dur;
            int chan;
        };
        struct BufDesc {
            float *data;
            size_t frames;
        };
        static std::queue<Job> jobQ;
        static std::mutex qMut;
        static std::unique_ptr<std::thread> worker;
        static constexpr size_t maxBufs = 16;
        static std::array<BufDesc, maxBufs> bufs;
        static int numBufs;
        static bool shouldQuit;
        static constexpr int sleepPeriodMs = 100;
        static int sampleRate;
        static constexpr int ioBufFrames = 1024;

        static int secToFrame(float seconds);

    private:
        static void requestJob(Job &job);

    public:
        // initialize with sample rate
        static void init(int sr);

        // register a buffer to manage.
        // returns index to be used in work requests
        static int registerBuffer(float *data, size_t frames);

        // clear a portion of a mono buffer
        static void requestClear(size_t idx, float start = 0, float dur = -1);

        // read mono soundfile to mono buffer
        static void
        requestReadMono(size_t idx, std::string path, float startSrc = 0, float startDst = 0, float dur = -1,
                        int chanSrc = 0);

        // read and de-interleave stereo soundfile to 2x mono buffers
        static void
        requestReadStereo(size_t idx0, size_t idx1, std::string path, float startSrc = 0, float startDst = 0,
                          float dur = -1);

        // write mono buf to mono soundfile
        static void requestWriteMono(size_t idx, std::string path, float start = 0, float dur = -1);

        // write and interleave two mono buffers to one stereo file
        static void requestWriteStereo(size_t idx0, size_t idx1, std::string path, float start = 0, float dur = -1);

    private:
        static void workLoop();

        static void clearBuffer(BufDesc &buf, float start = 0, float dur = -1);

        static void readBufferMono(const std::string &path, BufDesc &buf,
                                   float startSrc = 0, float startDst = 0, float dur = -1, int chanSrc = 0) noexcept;

        static void readBufferStereo(const std::string &path, BufDesc &buf0, BufDesc &buf1,
                                     float startSrc = 0, float startDst = 0, float dur = -1) noexcept;

        static void writeBufferMono(const std::string &path, BufDesc &buf,
                                    float start = 0, float dur = -1) noexcept;

        static void writeBufferStereo(const std::string &path, BufDesc &buf0, BufDesc &buf1,
                                      float start = 0, float dur = -1) noexcept;

    };

}

#endif //CRONE_BUFMANAGER_H
