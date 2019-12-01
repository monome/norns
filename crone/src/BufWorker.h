//
// Created by emb on 11/30/19.
//

#ifndef CRONE_BUFMANAGER_H
#define CRONE_BUFMANAGER_H

#include <atomic>
#include <thread>
#include <mutex>
//#include <queue>
#include <boost/lockfree/spsc_queue.hpp>

namespace crone {

    // class for asynchronous management of mono audio buffers
    class BufWorker {

        enum class WorkType {
            Clear,
            ReadMono, ReadStereo,
            WriteMono, WriteStereo
        };
        struct Work {
            WorkType type;
            int bufIdx[2];
            std::string path;
            float start;
            float dur;
        };
        struct BufDesc {
            float *data;
            size_t frames;
        };

        static boost::lockfree::spsc_queue<Work> workQ;
        static std::thread worker;
        static constexpr size_t maxBufs = 16;
        static std::array<BufDesc, maxBufs> bufs;
        static int numBufs;
        static bool shouldQuit;
        static constexpr int sleepPeriodMs = 100;
        static int sampleRate;
        static int secToFrame(float seconds);


    public:
        static void init(int sr);

        // register a buffer to manage.
        // returns index to be used in work requests
        static int registerBuffer(float *data, size_t frames);

        // read mono soundfile to mono buffer
        static void requestReadMono(int idx, std::string path, float start = 0, float dur = -1);

        // read and de-interleave stereo soundfile to 2x mono buffers
        static void requestReadStereo(int idx0, int idx1, std::string path, float start = 0, float dur = -1);

        // write mono buf to mono soundfile
        static void requestWriteMono(int idx, std::string path, float start = 0, float dur = -1);

        // write and interleave two mono buffers to one stereo file
        static void requestWriteStereo(int idx0, int idx1, std::string path, float start = 0, float dur = -1);

    private:
        static void workLoop();
        static void readBufferMono(std::string path, BufDesc &buf,
                float startSrc=0, float startDst=0, float dur=-1, int chanSrc=0);

        static void readBufferStereo(std::string path, BufDesc &buf0, BufDesc &buf1,
                                   float startSrc=0, float startDst=0, float dur=-1);

        static void writeBufferMono(std::string path, BufDesc &buf,
                                   float startSrc=0, float startDst=0, float dur=-1);

        static void writeBufferStereo(std::string path, BufDesc &buf0, BufDesc &buf1,
                                     float startSrc=0, float startDst=0, float dur=-1);

    };

}

#endif //CRONE_BUFMANAGER_H
