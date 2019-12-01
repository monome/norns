//
// Created by emb on 11/30/19.
//

#include <sndfile.hh>
#include <iostream>
#include "BufWorker.h"

// clamp unsigned int to upper bound, inclusive
static inline void clamp(size_t &x, const size_t a) {
    if (x > a) { x = a; }
}

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

void BufWorker::requestReadMono(int idx, std::string path, float start, float dur) {
    BufWorker::Work work{BufWorker::WorkType::ReadMono, {idx, idx}, path, start, dur};
    workQ.push(work);
}

void BufWorker::requestReadStereo(int idx0, int idx1, std::string path, float start, float dur) {
    BufWorker::Work work{BufWorker::WorkType::ReadStereo, {idx0, idx1}, path, start, dur};
    workQ.push(work);
}

void BufWorker::requestWriteMono(int idx, std::string path, float start, float dur) {
    BufWorker::Work work{BufWorker::WorkType::WriteMono, {idx, idx}, path, start, dur};
    workQ.push(work);
}

void BufWorker::requestWriteStereo(int idx0, int idx1, std::string path, float start, float dur) {
    BufWorker::Work work{BufWorker::WorkType::WriteStereo, {idx0, idx1}, path, start, dur};
    workQ.push(work);
}


void BufWorker::workLoop() {
    while (!shouldQuit) {
        if (workQ.read_available() > 0) {
            Work work;
            workQ.pop(&work);
            switch (work.type) {
                case WorkType::ReadMono:
                    data = bufs[work.bufIdx].data;
                    frames =
                    break;
                case WorkType::ReadStereo:
                    break;
                case WorkType::WriteMono:
                    break;
                case WorkType::WriteStereo:
                    break;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepPeriodMs));
        }
    }
}

void BufWorker::init(int sr) {
    sampleRate = sr;
    worker = std::thread(BufWorker::workLoop);
}

int BufWorker::secToFrame(float seconds) {
    return static_cast<int>(seconds * (float) sampleRate);
}


//////////////////


void BufWorker::readBufferMono(std::string path, BufDesc &buf,
                               float startSrc, float startDst, float dur, int chanSrc) {
    SndfileHandle file(path);

    if (file.frames() < 1) {
        std::cerr << "readBufferMono(): empty / missing file: " << path << std::endl;
        return;
    }

    size_t bufFrames = buf.frames;


    size_t frSrc = secToFrame(startSrc);
    clamp(frSrc, bufFrames - 1);

    size_t frDst = secToFrame(startDst);
    clamp(frDst, bufFrames - 1);

    size_t frDur;
    if (dur < 0.f) {
        auto maxDurSrc = file.frames() - frSrc;
        auto maxDurDst = file.frames() - frDst;
        frDur = maxDurSrc > maxDurDst ? maxDurDst : maxDurSrc;
    } else {
        frDur = secToFrame(dur);
    }


    // FIXME: perform the seek in blocks!
    auto numSrcChan = file.channels();
    std::unique_ptr<float[]> frBuf(new float[numSrcChan]);
    chanSrc = std::min(numSrcChan - 1, std::max(0, chanSrc));

    for (size_t fr = 0; fr < frDur; ++fr) {
        file.seek(frSrc, SEEK_SET);
        file.read(frBuf.get(), numSrcChan);
        buf.data[frDst] = frBuf[chanSrc];
        ++frSrc;
        if (++frDst >= bufFrames) {
            std::cerr << "readBufferMono(): exceeded buffer size; aborting" << std::endl;
            return;
        }
    }
}

//void BufWorker::readBufferStereo(const std::string &path, float startTimeSrc, float startTimeDst, float dur) {
//    SndfileHandle file(path);
//
//    if (file.frames() < 1) {
//        std::cerr << "SoftCutClient::readBufferStereo(): empty / missing file: " << path << std::endl;
//        return;
//    }
//
//    size_t frSrc = secToFrame(startTimeSrc);
//    clamp(frSrc, BufFrames - 1);
//
//    size_t frDst = secToFrame(startTimeDst);
//    clamp(frDst, BufFrames - 1);
//
//    size_t frDur;
//    if (dur < 0.f) {
//        auto maxDurSrc = file.frames() - frSrc;
//        auto maxDurDst = file.frames() - frDst;
//        frDur = maxDurSrc > maxDurDst ? maxDurDst : maxDurSrc;
//    } else {
//        frDur = secToFrame(dur);
//    }
//
//    auto numSrcChan = file.channels();
//    if (numSrcChan < 2) {
//        std::cerr << "SoftCutClient::readBufferStereo(): not enough channels in source; aborting" << std::endl;
//        return;
//    }
//    std::unique_ptr<float[]> frBuf(new float[numSrcChan]);
//
//    for (size_t fr = 0; fr < frDur; ++fr) {
//        // FIXME: don't seek every frame with libsndfile! it is slow
//        file.seek(frSrc, SEEK_SET);
//        file.read(frBuf.get(), numSrcChan);
//        buf[0][frDst] = frBuf[0];
//        buf[1][frDst] = frBuf[1];
//        frSrc++;
//        if (++frDst >= BufFrames) {
//            std::cerr << "SoftCutClient::readBufferStereo(): exceeded buffer size; aborting" << std::endl;
//            return;
//        }
//    }
//}
//
//
//void BufWorker::writeBufferMono(const std::string &path, float start = 0, float dur = -1, int chan = 0) {
//    // FIXME: use the cpp sndfile interface for tidiness..
//    SF_INFO sf_info;
//    SNDFILE *file;
//
//    sf_info.samplerate = 48000;
//    sf_info.channels = 1;
//    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
//
//    if ((file = sf_open(path.c_str(), SFM_WRITE, &sf_info)) == NULL) {
//        char errstr[256];
//        sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
//        std::cerr << "cannot open sndfile" << path << " for writing (" << errstr << ")" << std::endl;
//        return;
//    }
//
//    std::cerr << "SoftCutClient::writeBufferMono(): opened file for writing: " << path << std::endl;
//
//    size_t frSrc = secToFrame(start);
//    clamp(frSrc, BufFrames - 1);
//
//    size_t frDur;
//    if (dur < 0.f) {
//        // FIXME: should check available disk space?
//        frDur = BufFrames - frSrc;
//    } else {
//        frDur = secToFrame(dur);
//    }
//    /// FIXME: write frames in blocks
//    /// for now, write one frame at a time... :/
//
//    size_t nf = 0;
//    while (nf < frDur) {
//        if (sf_writef_float(file, buf[chan] + frSrc, 1) != 1) {
//            std::cerr << "SoftCutClient::writeBufferMono(): write failed (disk space?) after " << nf << " frames"
//                      << std::endl;
//            break;
//        }
//        ++frSrc;
//        ++nf;
//    }
//
//    sf_close(file);
//    std::cerr << "SoftCutClient::writeBufferMono(): done; wrote " << nf << " frames" << std::endl;
//}
//
//
//void BufWorker::writeBufferStereo(const std::string &path, float start = 0, float dur = -1) {
//    SF_INFO sf_info;
//    SNDFILE *file;
//
//    sf_info.samplerate = 48000;
//    sf_info.channels = 2;
//    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
//
//    if ((file = sf_open(path.c_str(), SFM_WRITE, &sf_info)) == NULL) {
//        char errstr[256];
//        sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
//        std::cerr << "cannot open sndfile" << path << " for writing (" << errstr << ")" << std::endl;
//        return;
//    }
//
//    std::cerr << "SoftCutClient::writeBufferStereo(): opened file for writing: " << path << std::endl;
//
//    float frameBuf[2];
//
//    size_t frSrc = secToFrame(start);
//    clamp(frSrc, BufFrames - 1);
//
//    size_t frDur;
//    if (dur < 0.f) {
//        // FIXME: should check available disk space?
//        frDur = BufFrames - frSrc;
//    } else {
//        frDur = secToFrame(dur);
//    }
//    /// FIXME: write frames in blocks
//    /// for now, write one frame at a time... :/
//    size_t nf = 0;
//    while (nf < frDur) {
//        frameBuf[0] = buf[0][frSrc];
//        frameBuf[1] = buf[1][frSrc];
//        if (sf_writef_float(file, frameBuf, 1) != 1) {
//            std::cerr << "SoftCutClient::writeBufferStereo(): write failed (disk space?) after " << nf << " frames"
//                      << std::endl;
//            break;
//        }
//        ++frSrc;
//        ++nf;
//    }
//
//    sf_close(file);
//    std::cerr << "SoftCutClient::writeBufferStereo(): done; wrote " << nf << " frames" << std::endl;
//}
//
//
