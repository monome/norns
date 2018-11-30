//
// Created by emb on 11/30/18.
//

#ifndef CRONE_POLL_H
#define CRONE_POLL_H

#include <string>
#include <thread>
#include <functional>
#include <atomic>

#include <lo/lo.h>

class Poll {

public:
    explicit Poll(std::string name);
    // for now, callbacks shall be responsible for actually sending OSC
    // this makes it easier to leave the message format open.
    typedef std::function<void(lo_address)> Callback;
    void setCallback(Callback cb);
    void start();
    void stop();
    void setPeriod(int ms);
    
private:
    Callback cb;
    std::atomic<int> period;
    std::unique_ptr<std::thread> th;
    std::string path;
    lo_address addr;
};


#endif //CRONE_POLL_H
