//
// Created by emb on 11/30/19.
//

#include "BufWorker.h"

#include <utility>

using namespace crone;

std::thread worker;
boost::lockfree::spsc_queue<BufWorker::Work> BufWorker::workQ;
std::array<BufWorker::BufDesc, BufWorker::maxBufs> BufWorker::bufs;
int BufWorker::numBufs = 0;
bool BufWorker::shouldQuit = false;

int BufWorker::registerBuffer(float *data, size_t frames) {
    int n = numBufs++;
    bufs[n].data = data;
    bufs[n].frames = frames;
    return n;
}

void BufWorker::requestReadMono(int idx[2], std::string path) {
    BufWorker::Work work { BufWorker::WorkType::ReadMono, {idx[0], idx[1]}, path };
    workQ.push(work);
}

void BufWorker::requestReadStereo(int idx[2], std::string path) {
    BufWorker::Work work { BufWorker::WorkType::ReadStereo, {idx[0], idx[1]}, path };
    workQ.push(work);
}

void BufWorker::requestWriteMono(int idx[2], std::string path) {
    BufWorker::Work work { BufWorker::WorkType::WriteMono, {idx[0], idx[1]}, path };
    workQ.push(work);
}


void BufWorker::requestWriteStereo(int idx[2], std::string path) {
    BufWorker::Work work { BufWorker::WorkType::WriteStereo, {idx[0], idx[1]}, path };
    workQ.push(work);
}

void BufWorker::workLoop() {
    while(!shouldQuit) {
        if (workQ.read_available() > 0) {
            auto work = workQ.pop();

        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepPeriodMs));
        }
    }
}

void BufWorker::init() {
    worker = std::thread(BufWorker::workLoop);
}
