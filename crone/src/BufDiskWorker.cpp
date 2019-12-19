//
// Created by emb on 11/30/19.
//

#include <sndfile.hh>
#include <iostream>
#include <utility>

#include "BufDiskWorker.h"
#include "OscInterface.h"

using namespace crone;

std::unique_ptr<std::thread> BufDiskWorker::worker = nullptr;
boost::lockfree::spsc_queue<BufDiskWorker::Job> BufDiskWorker::jobQ(BufDiskWorker::maxJobs);
std::array<BufDiskWorker::BufDesc, BufDiskWorker::maxBufs> BufDiskWorker::bufs;
int BufDiskWorker::numBufs = 0;
bool BufDiskWorker::shouldQuit = false;
int BufDiskWorker::sampleRate = 48000;


// clamp unsigned int to upper bound, inclusive
static inline void clamp(size_t &x, const size_t a) {
    if (x > a) { x = a; }
}


int BufDiskWorker::registerBuffer(float *data, size_t frames) {
    int n = numBufs++;
    bufs[n].data = data;
    bufs[n].frames = frames;
    return n;
}

void BufDiskWorker::requestClear(size_t idx, float start, float dur) {
    BufDiskWorker::Job job{std::time(nullptr), BufDiskWorker::JobType::Clear, {idx, 0}, "", 0, start, dur, 0};
    jobQ.push(job);
}

void
BufDiskWorker::requestReadMono(size_t idx, std::string path, float startSrc, float startDst, float dur, int chanSrc) {
    BufDiskWorker::Job job{std::time(nullptr), BufDiskWorker::JobType::ReadMono, {idx, 0}, std::move(path), startSrc, startDst, dur,
                           chanSrc};
    jobQ.push(job);
}

void BufDiskWorker::requestReadStereo(size_t idx0, size_t idx1, std::string path, float startSrc, float startDst,
                                      float dur) {
    BufDiskWorker::Job job{std::time(nullptr), BufDiskWorker::JobType::ReadStereo, {idx0, idx1}, std::move(path), startSrc, startDst, dur,
                           0};
    jobQ.push(job);
}

void BufDiskWorker::requestWriteMono(size_t idx, std::string path, float start, float dur) {
    BufDiskWorker::Job job{std::time(nullptr), BufDiskWorker::JobType::WriteMono, {idx, 0}, std::move(path), start, start, dur, 0};
    jobQ.push(job);
}

void BufDiskWorker::requestWriteStereo(size_t idx0, size_t idx1, std::string path, float start, float dur) {
    BufDiskWorker::Job job{std::time(nullptr), BufDiskWorker::JobType::WriteStereo, {idx0, idx1}, std::move(path), start, start, dur, 0};
    jobQ.push(job);
}


void BufDiskWorker::workLoop() {
    while (!shouldQuit) {
        if (jobQ.read_available() > 0) {
            Job job;
            jobQ.pop(&job);

#if 1
            std::cerr << "BufDiskWorker handling job; type = " << (int)job.type
                      << "; path = " << job.path
                      << "; startSrc = " << job.startSrc
                      << "; startDst = " << job.startDst
                      << "; dur = " << job.dur
                      << "; chanSrc = " << job.chan
                      << std::endl;
#endif

            switch (job.type) {
                case JobType::Clear:
                    clearBuffer(bufs[job.bufIdx[0], job.startDst, job.dur]);
                    break;
                case JobType::ReadMono:
                    readBufferMono(job.path, bufs[job.bufIdx[0]], job.startSrc, job.startDst, job.dur, job.chan);
                    break;
                case JobType::ReadStereo:
                    readBufferStereo(job.path, bufs[job.bufIdx[0]], bufs[job.bufIdx[1]], job.startSrc, job.startDst, job.dur);
                    break;
                case JobType::WriteMono:
                    writeBufferMono(job.path, bufs[job.bufIdx[0]], job.startSrc, job.dur);
                    break;
                case JobType::WriteStereo:
                    writeBufferStereo(job.path, bufs[job.bufIdx[0]], bufs[job.bufIdx[1]], job.startSrc, job.dur);
                    break;
            }
            OscInterface::notifyBufferJobComplete(job);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepPeriodMs));
        }
    }
}

void BufDiskWorker::init(int sr) {
    sampleRate = sr;
    if (worker == nullptr) {
        worker = std::make_unique<std::thread>(std::thread(BufDiskWorker::workLoop));
        worker->detach();
    }
}

int BufDiskWorker::secToFrame(float seconds) {
    return static_cast<int>(seconds * (float) sampleRate);
}

//------------------------
//---- private buffer routines

void BufDiskWorker::clearBuffer(BufDesc &buf, float start, float dur) {
    size_t frA = secToFrame(start);
    clamp(frA, buf.frames - 1);
    size_t frB;
    if (dur < 0) {
        frB = buf.frames;
    } else {
        frB = frA + secToFrame(dur);
    }
    clamp(frB, buf.frames);
    for (size_t i = frA; i < frB; ++i) {
        buf.data[i] = 0.f;
    }
}

void BufDiskWorker::readBufferMono(const std::string &path, BufDesc &buf,
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
        auto maxDurDst = bufFrames - frDst;
        frDur = maxDurSrc > maxDurDst ? maxDurDst : maxDurSrc;
    } else {
        frDur = secToFrame(dur);
    }

    // FIXME: perform the seek in blocks!
    auto numSrcChan = file.channels();
    std::unique_ptr<float[]> frBuf(new float[numSrcChan]);
    chanSrc = std::min(numSrcChan - 1, std::max(0, chanSrc));
    std::cerr << "reading soundfile channel " << chanSrc << std::endl;

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

void BufDiskWorker::readBufferStereo(const std::string &path, BufDesc &buf0, BufDesc &buf1,
                                     float startTimeSrc, float startTimeDst, float dur) {
    SndfileHandle file(path);

    if (file.frames() < 1) {
        std::cerr << "SoftCutClient::readBufferStereo(): empty / missing file: " << path << std::endl;
        return;
    }

    size_t bufFrames = buf0.frames < buf1.frames ? buf0.frames : buf1.frames;

    size_t frSrc = secToFrame(startTimeSrc);
    clamp(frSrc, bufFrames - 1);

    size_t frDst = secToFrame(startTimeDst);
    clamp(frDst, bufFrames - 1);

    size_t frDur;
    if (dur < 0.f) {
        auto maxDurSrc = file.frames() - frSrc;
        auto maxDurDst = bufFrames - frDst;
        frDur = maxDurSrc > maxDurDst ? maxDurDst : maxDurSrc;
    } else {
        frDur = secToFrame(dur);
    }

    auto numSrcChan = file.channels();
    if (numSrcChan < 2) {
        std::cerr << "SoftCutClient::readBufferStereo(): not enough channels in source; aborting" << std::endl;
        return;
    }
    std::unique_ptr<float[]> frBuf(new float[numSrcChan]);

    for (size_t fr = 0; fr < frDur; ++fr) {
        // FIXME: don't seek every frame with libsndfile! it is slow
        file.seek(frSrc, SEEK_SET);
        file.read(frBuf.get(), numSrcChan);
        buf0.data[frDst] = frBuf[0];
        buf1.data[frDst] = frBuf[1];
        frSrc++;
        if (++frDst >= bufFrames) {
            std::cerr << "SoftCutClient::readBufferStereo(): exceeded buffer size; aborting" << std::endl;
            return;
        }
    }
}

void BufDiskWorker::writeBufferMono(const std::string &path, BufDesc &buf, float start, float dur) {
    // FIXME: use the cpp sndfile interface for tidiness..
    SF_INFO sf_info;
    SNDFILE *file;

    sf_info.samplerate = 48000;
    sf_info.channels = 1;
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

    if ((file = sf_open(path.c_str(), SFM_WRITE, &sf_info)) == NULL) {
        char errstr[256];
        sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
        std::cerr << "cannot open sndfile" << path << " for writing (" << errstr << ")" << std::endl;
        return;
    }

    sf_command(file, SFC_SET_CLIPPING, NULL, SF_TRUE);
    std::cerr << "BufDiskWorker::writeBufferMono(): opened file for writing: " << path << std::endl;

    size_t frSrc = secToFrame(start);

    size_t bufFrames = buf.frames;
    clamp(frSrc, bufFrames - 1);

    size_t frDur;
    if (dur < 0.f) {
        // FIXME: should check available disk space?
        frDur = bufFrames - frSrc;
    } else {
        frDur = secToFrame(dur);
    }
    /// FIXME: write frames in blocks
    /// for now, write one frame at a time... :/

    size_t nf = 0;
    while (nf < frDur) {
        if (sf_writef_float(file, buf.data + frSrc, 1) != 1) {
            std::cerr << "BufDiskWorker::writeBufferMono(): write failed (disk space?) after " << nf << " frames"
                      << std::endl;
            break;
        }
        ++frSrc;
        ++nf;
    }

    sf_close(file);
    std::cerr << "BufDiskWorker::writeBufferMono(): done; wrote " << nf << " frames" << std::endl;
}

void BufDiskWorker::writeBufferStereo(const std::string &path, BufDesc &buf0, BufDesc &buf1, float start, float dur) {
    SF_INFO sf_info;
    SNDFILE *file;

    sf_info.samplerate = 48000;
    sf_info.channels = 2;
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

    if ((file = sf_open(path.c_str(), SFM_WRITE, &sf_info)) == NULL) {
        char errstr[256];
        sf_error_str(nullptr, errstr, sizeof(errstr) - 1);
        std::cerr << "cannot open sndfile" << path << " for writing (" << errstr << ")" << std::endl;
        return;
    }

    sf_command(file, SFC_SET_CLIPPING, NULL, SF_TRUE);
    std::cerr << "SoftCutClient::writeBufferStereo(): opened file for writing: " << path << std::endl;

    float frameBuf[2];

    size_t frSrc = secToFrame(start);

    size_t bufFrames = buf0.frames < buf1.frames ? buf0.frames : buf1.frames;
    clamp(frSrc, bufFrames - 1);

    size_t frDur;
    if (dur < 0.f) {
        // FIXME: should check available disk space?
        frDur = bufFrames - frSrc;
    } else {
        frDur = secToFrame(dur);
    }
    /// FIXME: write frames in blocks
    /// for now, write one frame at a time... :/
    size_t nf = 0;
    while (nf < frDur) {
        frameBuf[0] = buf0.data[frSrc];
        frameBuf[1] = buf0.data[frSrc];
        if (sf_writef_float(file, frameBuf, 1) != 1) {
            std::cerr << "SoftCutClient::writeBufferStereo(): write failed (disk space?) after " << nf << " frames"
                      << std::endl;
            break;
        }
        ++frSrc;
        ++nf;
    }

    sf_close(file);
    std::cerr << "SoftCutClient::writeBufferStereo(): done; wrote " << nf << " frames" << std::endl;
}
