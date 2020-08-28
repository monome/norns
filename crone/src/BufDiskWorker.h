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
#include <condition_variable>
#include <queue>
#include <memory>
#include <functional>

namespace crone {
    // class for asynchronous management of mono audio buffers
    class BufDiskWorker {
    public:
        typedef std::function<void(float secPerSample, float start, size_t count, float* samples)> RenderCallback;

    private:
        enum class JobType {
            Clear, ClearWithFade, Copy,
            ReadMono, ReadStereo,
            WriteMono, WriteStereo,
            Render,
        };
        struct Job {
            JobType type;
            size_t bufIdx[2];
            std::string path;
            float startSrc;
            float startDst;
            float dur;
            int chan;
            float fadeTime;
            float preserve;
            bool reverse;
            int samples;
            RenderCallback renderCallback;
        };
        struct BufDesc {
            float *data;
            size_t frames;
        };
        static std::queue<Job> jobQ;
        static std::mutex qMut;
        static std::condition_variable qCv;
        static std::unique_ptr<std::thread> worker;
        static constexpr size_t maxBufs = 16;
        static std::array<BufDesc, maxBufs> bufs;
        static int numBufs;
        static bool shouldQuit;
        static constexpr int sleepPeriodMs = 100;
        static int sampleRate;
        static constexpr int ioBufFrames = 1024;

        static int secToFrame(float seconds);
        static float raisedCosFade(float unitphase);
        static float mixFade(float x, float y, float a, float b);

        template <typename Step>
        static void copyLoop(float* dst, const float* src,
                             size_t frDur, size_t frFadeTime,
                             float preserve, float x, float phi,
                             Step&& step);

        static void requestJob(Job &job);

    public:
        // initialize with sample rate
        static void init(int sr);

        // register a buffer to manage.
        // returns index to be used in work requests
        static int registerBuffer(float *data, size_t frames);

        // clear a portion of a mono buffer
        static void requestClear(size_t idx, float start = 0, float dur = -1);

        static void requestClearWithFade(size_t idx, float start = 0, float dur = -1,
                                         float fadeTime = 0, float preserve = 0);

        static void requestCopy(size_t srcIdx, size_t dstIdx,
                                float srcStart = 0, float dstStart = 0, float dur = -1,
                                float fadeTime = 0, float preserve = 0, bool reverse = false);

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

        static void requestRender(size_t idx, float start, float dur, int count, RenderCallback callback);

    private:
        static void workLoop();

        static void clearBuffer(BufDesc &buf, float start = 0, float dur = -1);

        static void clearBufferWithFade(BufDesc &buf, float start = 0, float dur = -1, float fadeTime = 0, float preserve = 0);

        static void copyBuffer(BufDesc &buf0, BufDesc &buf1,
                               float srcStart = 0, float dstStart = 0, float dur = -1,
                               float fadeTime = 0, float preserve = 0, bool reverse = false);

        static void readBufferMono(const std::string &path, BufDesc &buf,
                                   float startSrc = 0, float startDst = 0, float dur = -1, int chanSrc = 0) noexcept;

        static void readBufferStereo(const std::string &path, BufDesc &buf0, BufDesc &buf1,
                                     float startSrc = 0, float startDst = 0, float dur = -1) noexcept;

        static void writeBufferMono(const std::string &path, BufDesc &buf,
                                    float start = 0, float dur = -1) noexcept;

        static void writeBufferStereo(const std::string &path, BufDesc &buf0, BufDesc &buf1,
                                      float start = 0, float dur = -1) noexcept;

        static void render(BufDesc &buf, float start, float dur, size_t samples, RenderCallback callback);
    };

}

#endif //CRONE_BUFMANAGER_H
