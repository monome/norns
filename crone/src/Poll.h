//
// Created by emb on 11/30/18.
//

#ifndef CRONE_POLL_H
#define CRONE_POLL_H

#include <string>
#include <thread>
#include <functional>
#include <atomic>

#include <sstream>
#include <chrono>
#include <utility>

#include <lo/lo.h>

class Poll {

public:
    typedef std::function<void(const char*)> Callback;

    explicit Poll(std::string name) {
        std::ostringstream os;
        os << "/poll/" << name;
        path = os.str();
    }

    void setCallback(Callback c) {
        cb = std::move(c);
    }

    void start() {
        th = std::make_unique<std::thread> (std::thread(
                [this] {
                    while (true) {
                        this->cb(path.c_str());
                        std::this_thread::sleep_for(std::chrono::milliseconds(period));
                    }
                }));
        th->detach();
    }

    void stop() {
        // destroying the std::thread object should terminate the thread..?
        th.reset();
    }

    void setPeriod(int ms) {
        period = ms;
    }

private:
    Callback cb;
    std::atomic<int> period;
    std::unique_ptr<std::thread> th;
    std::string path;
    lo_address addr;
};


#endif //CRONE_POLL_H
