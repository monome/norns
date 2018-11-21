//
// Created by emb on 11/18/18.
//

#include <iostream>
#include <chrono>
#include <thread>

#include "JackClient.h"
#include "OscInterface.h"


static inline void sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main() {
    using namespace crone;

    JackClient::setup();
    JackClient::start();

    OscInterface::init();

    while(!OscInterface::shouldQuit())  {
        sleep(100);
    }

    JackClient::stop();
    JackClient::cleanup();

}