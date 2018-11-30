//
// Created by emb on 11/30/18.
//


#include <sstream>
#include <chrono>
#include <utility>

#include "Poll.h"

Poll::Poll(std::string name) {
    addr = lo_address_new("127.0.0.1", "8888");
    std::ostringstream os;
    os << "/poll/" << name;
    path = os.str();
}

void Poll::setCallback(Callback c) {
    cb = std::move(c);
}

void Poll::start() {
    th = std::make_unique<std::thread>(
            std::thread([this] {
                this->cb(this->addr);
                std::this_thread::sleep_for(std::chrono::milliseconds(period));
            }));
}

void Poll::stop() {
    // destroying the std::thread object should terminate the thread..?
    th.reset();
}

void Poll::setPeriod(int ms) {
    period = ms;
}