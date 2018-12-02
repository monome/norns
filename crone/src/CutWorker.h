
#include <thread>
#include <mutex>
#include <condition_variable>

#include "Bus.h"
#include "softcut/SoftCut.h"

namespace crone {

    template<int NumCuts, int BufSize>
    class CutWorker {
        friend class AudioMain;

    private:
        typedef Bus<1, BufSize> MonoBus;
        typedef Bus<2, BufSize> StereoBus;

    protected:
        softcut::SoftCut<NumCuts> cut;

        MonoBus cut_in[NumCuts];
        MonoBus cut_out[NumCuts];

        bool enabled[NumCuts];
        //    StereoBus cut_mix;

        // LogRamp cut_amp[SOFTCUT_COUNT];
        // LogRamp cut_pan[SOFTCUT_COUNT];

    private:
        std::atomic<bool> start;
        volatile bool done;
        volatile bool quit;
        size_t numFrames;
        std::thread t;
        std::mutex m;
        std::condition_variable cv;

    private:
        // call from worker
        void workLoop() {

            while(true) {
                /// FIXME: need stopping condition?
                std::unique_lock<std::mutex> lk(m);
                cv.wait(lk, [this] { return this->start; });
                if(quit) { break; }
                start = false;

                process();

                done = true;
                lk.unlock(); // explicit unlock for spurious wakeup
                cv.notify_one();
            }
        }

        void process() {
            // process softcuts (overwrites output bus)
            for(int v=0; v<NumCuts; ++v) {
                if (!enabled[v]) {
                    continue;
                }
                cut.processBlock(v, cut_in[v].buf[0], cut_out[v].buf[0], static_cast<int>(numFrames));
            }
        }


        // call from main
        void startProcess(size_t nf) {
            {
                std::lock_guard<std::mutex> lk(m);
                numFrames = nf;
                start = true;
            }
            cv.notify_one();
        }

        void waitForDone() {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [this]{ return this->done; });
            done = false;
        }

    public:
        CutWorker() : start(false), done(false), quit(false),
                      t([this] { this->workLoop(); })
        {
            for (auto &b: enabled) { b = false; }
        }
    };
}
