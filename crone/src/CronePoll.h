//
// Created by emb on 11/30/18.
//

#ifndef CRONE_POLL_H
#define CRONE_POLL_H

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

class Poll {

  public:
    typedef std::function<void(const char *)> Callback;

    explicit Poll(std::string name) {
        std::ostringstream os;
        os << "/poll/" << name;
        path = os.str();
    }

    ~Poll() {
        stop();
    }

    void setCallback(Callback c) {
        cb = std::move(c);
    }

    void start() {
        assert(this->cb && "Poll callback must be set before starting");
        stop(); // ensure any existing thread is stopped safely

        shouldStop = false;
        th = std::thread([this] {
            while (!shouldStop) {
                this->cb(path.c_str());

                std::unique_lock<std::mutex> lock(mtx);
                // use cv.wait_for so that shouldStop can trigger with true immediately.
                cv.wait_for(lock, std::chrono::milliseconds(period.load()), [this] {
                    return shouldStop.load();
                });
            }
        });
    }

    void stop() {
        if (th.joinable()) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                shouldStop = true;
            }
            cv.notify_all();
            th.join();
        }
    }

    void setPeriod(int ms) {
        period = ms;
    }

  private:
    Callback cb;
    std::atomic<int> period{50};
    std::atomic<bool> shouldStop{true};
    std::thread th;
    std::string path;
    std::mutex mtx;
    std::condition_variable cv;
};

#endif // CRONE_POLL_H
