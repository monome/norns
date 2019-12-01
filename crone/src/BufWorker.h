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

        enum class WorkType { ReadMono, ReadStereo, WriteMono, WriteStereo };
        struct Work { WorkType type; int bufIdx[2]; std::string path; };
        struct BufDesc { float *data; size_t frames; };

        static boost::lockfree::spsc_queue<Work> workQ;
        static std::thread worker;
        static constexpr size_t maxBufs = 16;
        static std::array<BufDesc, maxBufs> bufs;
        static int numBufs;
        static bool shouldQuit;
        static constexpr int sleepPeriodMs = 100;



    public:
        static void init();

        // register a buffer to manage.
        // returns index to be used in work requests
        static int registerBuffer(float *data, size_t frames);

        static void requestReadMono(int idx[2], std::string path);
        static void requestReadStereo(int idx[2], std::string path);

        static void requestWriteMono(int idx[2], std::string path);
        static void requestWriteStereo(int idx[2], std::string path);

    private:
        static void workLoop();
    };

}

#endif //CRONE_BUFMANAGER_H
