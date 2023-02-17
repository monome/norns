//
// Created by emb on 11/30/19.
//


//-----------------------
//-- debugging
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <chrono>
//--------------

#include <sndfile.hh>
#include <array>
#include <cmath>
#include <thread>
#include <utility>
// -------------------
// for shared memory
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
// -------------------

#include "BufDiskWorker.h"

using namespace crone;

std::unique_ptr<std::thread> BufDiskWorker::worker = nullptr;
std::queue<BufDiskWorker::Job> BufDiskWorker::jobQ;
std::mutex BufDiskWorker::qMut;
std::condition_variable BufDiskWorker::qCv;

std::array<BufDiskWorker::BufDesc, BufDiskWorker::maxBufs> BufDiskWorker::bufs;
int BufDiskWorker::numBufs = 0;
bool BufDiskWorker::shouldQuit = false;
int BufDiskWorker::sampleRate = 48000;
int BufDiskWorker::fd = -1;

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

void BufDiskWorker::requestJob(BufDiskWorker::Job &job) {
    qMut.lock();
    jobQ.push(job);
    qMut.unlock();
    qCv.notify_one();
}

void BufDiskWorker::requestClear(size_t idx, float start, float dur) {
    BufDiskWorker::Job job{BufDiskWorker::JobType::Clear, {idx, 0}, "", 0, start, dur, 0};
    requestJob(job);
}

void BufDiskWorker::requestClearWithFade(size_t idx, float start, float dur,
                                         float fadeTime, float preserve) {
    BufDiskWorker::Job job{BufDiskWorker::JobType::ClearWithFade, {idx, 0}, "", 0, start, dur, 0, fadeTime, preserve};
    requestJob(job);
}

void BufDiskWorker::requestCopy(size_t srcIdx, size_t dstIdx,
                                float srcStart, float dstStart, float dur,
                                float fadeTime, float preserve, bool reverse) {
    BufDiskWorker::Job job{BufDiskWorker::JobType::Copy, {srcIdx, dstIdx}, "", srcStart, dstStart, dur, 0, fadeTime, preserve, 0.f, reverse};
    requestJob(job);
}

void
BufDiskWorker::requestReadMono(size_t idx, std::string path, float startSrc, float startDst, float dur, int chanSrc,
                               float preserve, float mix) {
    BufDiskWorker::Job job{BufDiskWorker::JobType::ReadMono, {idx, 0}, std::move(path), startSrc, startDst, dur,
                           chanSrc, 0.f, preserve, mix};
    requestJob(job);
}

void BufDiskWorker::requestReadStereo(size_t idx0, size_t idx1, std::string path,
                                      float startSrc, float startDst, float dur, float preserve, float mix) {
    BufDiskWorker::Job job{BufDiskWorker::JobType::ReadStereo, {idx0, idx1}, std::move(path), startSrc, startDst, dur,
                           0, 0.f, preserve, mix};
    requestJob(job);
}

void BufDiskWorker::requestWriteMono(size_t idx, std::string path, float start, float dur) {
    BufDiskWorker::Job job{BufDiskWorker::JobType::WriteMono, {idx, 0}, std::move(path), start, start, dur, 0};
    requestJob(job);
}

void BufDiskWorker::requestWriteStereo(size_t idx0, size_t idx1, std::string path,
                                       float start, float dur) {
    BufDiskWorker::Job job{BufDiskWorker::JobType::WriteStereo, {idx0, idx1}, std::move(path), start, start, dur, 0};
    requestJob(job);
}

void BufDiskWorker::requestRender(size_t idx, float start, float dur, int samples, RenderCallback callback) {
    BufDiskWorker::Job job{BufDiskWorker::JobType::Render, {idx, 0}, "", start, start, dur, 0, 0.f, 0.f, 0.f, false, samples, callback};
    requestJob(job);
}

void BufDiskWorker::requestProcess(size_t idx, float start, float dur, ProcessCallback processCallback) {
    BufDiskWorker::Job job{BufDiskWorker::JobType::Process, {idx, 0}, "", start, start, dur, 0, 0, 0, 1, false, 0, nullptr, processCallback };
    requestJob(job);
}

void BufDiskWorker::requestPoke(size_t idx, float start, float dur, DoneCallback doneCallback) {
    BufDiskWorker::Job job{BufDiskWorker::JobType::Poke, {idx, 0}, "", start, start, dur, 0, 0, 0, 1, false, 0, nullptr, nullptr, doneCallback};
    requestJob(job);
}

void BufDiskWorker::workLoop() {
    while (!shouldQuit) {
        Job job;
        {
            std::unique_lock<std::mutex> lock(qMut);
            qCv.wait(lock, [] { return !jobQ.empty(); });
            job = jobQ.front();
            jobQ.pop();
            lock.unlock();
            qCv.notify_one();
        }

        switch (job.type) {
            case JobType::Clear:
                clearBuffer(bufs[job.bufIdx[0]], job.startDst, job.dur);
                break;
            case JobType::ClearWithFade:
                clearBufferWithFade(bufs[job.bufIdx[0]], job.startDst, job.dur, job.fadeTime, job.preserve);
                break;
            case JobType::Copy:
                copyBuffer(bufs[job.bufIdx[0]], bufs[job.bufIdx[1]], job.startSrc, job.startDst, job.dur, job.fadeTime, job.preserve, job.reverse);
                break;
            case JobType::ReadMono:
                readBufferMono(job.path, bufs[job.bufIdx[0]], job.startSrc, job.startDst, job.dur, job.chan, job.preserve, job.mix);
                break;
            case JobType::ReadStereo:
                readBufferStereo(job.path, bufs[job.bufIdx[0]], bufs[job.bufIdx[1]], job.startSrc, job.startDst,
                                 job.dur, job.preserve, job.mix);
                break;
            case JobType::WriteMono:
                writeBufferMono(job.path, bufs[job.bufIdx[0]], job.startSrc, job.dur);
                break;
            case JobType::WriteStereo:
                writeBufferStereo(job.path, bufs[job.bufIdx[0]], bufs[job.bufIdx[1]], job.startSrc, job.dur);
                break;
            case JobType::Render:
                render(bufs[job.bufIdx[0]], job.startSrc, job.dur, (size_t)job.samples, job.renderCallback);
                break;
            case JobType::Process:
                process(bufs[job.bufIdx[0]], job.startSrc, job.dur, job.processCallback);
                break;
            case JobType::Poke:
                poke(bufs[job.bufIdx[0]], job.startSrc, job.dur, job.doneCallback);
        }
#if 0 // debug, timing
        auto ms_now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        auto ms_dur = ms_now - ms_start;
        std::cout << "job finished; elapsed time = " << ms_dur << " ms" << std::endl;
#endif
    }
    shm_unlink("BufDiskWorker_shm");
}

void BufDiskWorker::init(int sr) {
    sampleRate = sr;
    // don't really love using this as a magic word,
    // but I also don't love passing it around.
    fd = shm_open("BufDiskWorker_shm", O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (worker == nullptr) {
        worker = std::make_unique<std::thread>(std::thread(BufDiskWorker::workLoop));
        worker->detach();
    }
}

size_t BufDiskWorker::secToFrame(float seconds) {
    return static_cast<size_t>(seconds * (float) sampleRate);
}

float BufDiskWorker::raisedCosFade(float unitphase) {
    return 0.5f * (cosf(M_PI * (1.f + unitphase)) + 1.f);
}

float BufDiskWorker::mixFade(float x, float y, float a, float b) {
    return x * sinf(a * (float)M_PI_2) + y * sinf(b * (float) M_PI_2);
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

void BufDiskWorker::clearBufferWithFade(BufDesc &buf, float start, float dur,
                                        float fadeTime, float preserve) {
    size_t frStart = secToFrame(start);
    clamp(frStart, buf.frames - 1);
    size_t frDur;
    if (dur < 0) {
        frDur = buf.frames - frStart;
    } else {
        frDur = secToFrame(dur);
    }
    clamp(frDur, buf.frames - frStart);

    float x;
    float phi;
    size_t frFadeTime = secToFrame(fadeTime);
    if (frFadeTime > 0) {
        x = 0.f;
        phi = 1.f / frFadeTime;
        clamp(frFadeTime, frDur);
    } else {
        frFadeTime = 0;
        x = 1.f;
        phi = 0.f;
    }

    if (preserve > 1.f) { preserve = 1.f; }
    if (preserve < 0.f) { preserve = 0.f; }

    float zero = 0.f;
    copyLoop(buf.data + frStart,
             &zero,
             frDur, frFadeTime,
             preserve, x, phi,
             [](float*& d, const float*& s) { d++; });
}

void BufDiskWorker::copyBuffer(BufDesc &buf0, BufDesc &buf1,
                               float srcStart, float dstStart, float dur,
                               float fadeTime, float preserve, bool reverse) {
    size_t frSrcStart = secToFrame(srcStart);
    clamp(frSrcStart, buf0.frames - 1);
    size_t frDstStart = secToFrame(dstStart);
    clamp(frDstStart, buf1.frames - 1);

    size_t frDur;
    if (dur < 0) {
        frDur = buf0.frames - frSrcStart;
    } else {
        frDur = secToFrame(dur);
    }
    clamp(frDur, buf1.frames - frDstStart);

    if (preserve > 1.f) { preserve = 1.f; }
    if (preserve < 0.f) { preserve = 0.f; }

    float x;
    float phi;
    size_t frFadeTime = secToFrame(fadeTime);
    if (frFadeTime > 0) {
        x = 0.f;
        phi = 1.f / frFadeTime;
        clamp(frFadeTime, frDur);
    } else {
        frFadeTime = 0;
        x = 1.f;
        phi = 0.f;
    }

    if (reverse) {
        // reversing contents while copying into an overlapping region
        // is not possible without additional storage the size of the
        // overlap, so we don't handle this
        copyLoop(buf1.data + frDstStart,
                 buf0.data + frSrcStart + frDur - 1,
                 frDur, frFadeTime,
                 preserve, x, phi,
                 [](float*& d, const float*& s) { d++; s--; });
    } else {
        // source and destination regions might overlap, so we need to
        // imitate std::memmove - when src < dst start from the end and
        // move backwards
        if (frDstStart < frSrcStart) {
            copyLoop(buf1.data + frDstStart,
                     buf0.data + frSrcStart,
                     frDur, frFadeTime,
                     preserve, x, phi,
                     [](float*& d, const float*& s) { d++; s++; });
        } else {
            copyLoop(buf1.data + frDstStart + frDur - 1,
                     buf0.data + frSrcStart + frDur - 1,
                     frDur, frFadeTime,
                     preserve, x, phi,
                     [](float*& d, const float*& s) { d--; s--; });
        }
    }
}

template <typename Step>
void BufDiskWorker::copyLoop(float* dst, const float* src,
                             size_t frDur, size_t frFadeTime,
                             float preserve, float x, float phi,
                             Step&& update) {
    size_t i;
    float lambda;
    for (i = 0; i < frFadeTime; i++) {
        lambda = raisedCosFade(x);
        *dst = mixFade(*dst, *src,
                       1.f - lambda * (1.f - preserve), lambda);
        x += phi;
        update(dst, src);
    }
    for ( ; i < frDur - frFadeTime; i++) {
        *dst = preserve * *dst + *src;
        update(dst, src);
    }
    for ( ; i < frDur; i++) {
        lambda = raisedCosFade(x);
        *dst = mixFade(*dst, *src,
                       1.f - lambda * (1.f - preserve), lambda);
        x -= phi;
        update(dst, src);
    }
}

void BufDiskWorker::readBufferMono(const std::string &path, BufDesc &buf,
                                   float startSrc, float startDst, float dur, int chanSrc, float preserve, float mix)
noexcept {
    SndfileHandle file(path);

    if (file.frames() < 1) {
        std::cerr << "readBufferMono(): empty / missing file: " << path << std::endl;
        return;
    }

    size_t bufFrames = buf.frames;

    size_t frSrc = secToFrame(startSrc);
    clamp(frSrc, file.frames() - 1);

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

    auto numSrcChan = file.channels();
    chanSrc = std::min(numSrcChan - 1, std::max(0, chanSrc));

    auto *ioBuf = new float[numSrcChan * ioBufFrames];
    size_t numBlocks = frDur / ioBufFrames;
    size_t rem = frDur - (numBlocks * ioBufFrames);
    std::cout << "file contains " << file.frames() << " frames" << std::endl;
    std::cout << "reading " << numBlocks << " blocks and " << rem << " remainder frames..." << std::endl;
    for (size_t block = 0; block < numBlocks; ++block) {
        int res = file.seek(frSrc, SF_SEEK_SET);
        if (res == -1) {
            std::cerr << "error seeking to frame: " << frSrc << "; aborting read" << std::endl;
            goto cleanup;
        }
        file.readf(ioBuf, ioBufFrames);
        for (int fr = 0; fr < ioBufFrames; ++fr) {
            buf.data[frDst] = buf.data[frDst] * preserve + ioBuf[fr * numSrcChan + chanSrc] * mix;
            frDst++;
        }
        frSrc += ioBufFrames;
    }
    for (size_t i = 0; i < rem; ++i) {
        int res = file.seek(frSrc, SF_SEEK_SET);

        if (res == -1) {
            std::cerr << "error seeking to frame: " << frSrc << "; aborting read" << std::endl;
            goto cleanup;
        }
        file.read(ioBuf, numSrcChan);
        buf.data[frDst] = buf.data[frDst] * preserve + ioBuf[chanSrc] * mix;
        frDst++;
        frSrc++;
    }
    cleanup:
    delete[] ioBuf;
}

void BufDiskWorker::readBufferStereo(const std::string &path, BufDesc &buf0, BufDesc &buf1,
                                     float startTimeSrc, float startTimeDst, float dur,
                                     float preserve, float mix)
noexcept {
    SndfileHandle file(path);

    if (file.frames() < 1) {
        std::cerr << "SoftCutClient::readBufferStereo(): empty / missing file: " << path << std::endl;
        return;
    }

    size_t bufFrames = buf0.frames < buf1.frames ? buf0.frames : buf1.frames;

    size_t frSrc = secToFrame(startTimeSrc);
    clamp(frSrc, file.frames() - 1);

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

    auto *ioBuf = new float[numSrcChan * ioBufFrames];

    size_t numBlocks = frDur / ioBufFrames;
    size_t rem = frDur - (numBlocks * ioBufFrames);

    for (size_t block = 0; block < numBlocks; ++block) {
        int res = file.seek(frSrc, SF_SEEK_SET);
        if (res == -1) {
            std::cerr << "error seeking to frame: " << frSrc << "; aborting read" << std::endl;
            goto cleanup;
        }
        file.readf(ioBuf, ioBufFrames);
        for (int fr = 0; fr < ioBufFrames; ++fr) {
            buf0.data[frDst] = buf0.data[frDst] * preserve + ioBuf[fr * numSrcChan] * mix;
            buf1.data[frDst] = buf1.data[frDst] * preserve + ioBuf[fr * numSrcChan + 1] * mix;
            frDst++;
        }
        frSrc += ioBufFrames;
    }
    for (size_t i = 0; i < rem; ++i) {
        int res = file.seek(frSrc, SF_SEEK_SET);
        if (res == -1) {
            std::cerr << "error seeking to frame: " << frSrc << "; aborting read" << std::endl;
            goto cleanup;
        }
        file.read(ioBuf, numSrcChan);
        buf0.data[frDst] = buf0.data[frDst] * preserve + ioBuf[0] * mix;
        buf1.data[frDst] = buf1.data[frDst] * preserve + ioBuf[1] * mix;
        frDst++;
        frSrc++;
    }
    cleanup:
    delete[] ioBuf;
}

void BufDiskWorker::writeBufferMono(const std::string &path, BufDesc &buf, float start, float dur) noexcept {
    const int sr = 48000;
    const int channels = 1;
    const int format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

    SndfileHandle file(path, SFM_WRITE, format, channels, sr);

    if (not file) {
        std::cerr << "BufDiskWorker::writeBufferMono(): cannot open sndfile" << path << " for writing" << std::endl;
        return;
    }

    file.command(SFC_SET_CLIPPING, NULL, SF_TRUE);

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

    size_t numBlocks = frDur / ioBufFrames;
    size_t rem = frDur - (numBlocks * ioBufFrames);
    size_t nf = 0;
    float *pbuf = buf.data + frSrc;
    for (size_t block = 0; block < numBlocks; ++block) {
        size_t n = file.writef(pbuf, ioBufFrames);
        pbuf += ioBufFrames;
        nf += n;
        if (n != ioBufFrames) {
            std::cerr << "BufDiskWorker::writeBufferMono(): write aborted (disk space?) after " << nf << " frames"
                      << std::endl;
            return;
        }
    }

    for (size_t i = 0; i < rem; ++i) {
        if (file.writef(pbuf++, 1) != 1) {
            std::cerr << "BufDiskWorker::writeBufferMono(): write aborted (disk space?) after " << nf << " frames"
                      << std::endl;
            return;
        }
        ++nf;
    }
}

void BufDiskWorker::writeBufferStereo(const std::string &path, BufDesc &buf0, BufDesc &buf1, float start, float dur)
noexcept {
    const int sr = 48000;
    const int channels = 2;
    const int format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
    SndfileHandle file(path, SFM_WRITE, format, channels, sr);

    if (not file) {
        std::cerr << "ERROR: cannot open sndfile" << path << " for writing" << std::endl;
        return;
    }

    file.command(SFC_SET_CLIPPING, NULL, SF_TRUE);

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

    size_t numBlocks = frDur / ioBufFrames;
    size_t rem = frDur - (numBlocks * ioBufFrames);
    size_t nf = 0;

    auto *ioBuf = new float[ioBufFrames * 2];
    float *pbuf0 = buf0.data + frSrc;
    float *pbuf1 = buf1.data + frSrc;
    for (size_t block = 0; block < numBlocks; ++block) {
        float *pio = ioBuf;
        for (size_t fr = 0; fr < ioBufFrames; ++fr) {
            *pio++ = *pbuf0++;
            *pio++ = *pbuf1++;
        }
        size_t n = file.writef(ioBuf, ioBufFrames);
        nf += n;
        if (n != ioBufFrames) {
            std::cerr << "BufDiskWorker::writeBufferStereo(): write aborted (disk space?) after " << nf << " frames"
                      << std::endl;
            goto cleanup;
        }
        frSrc += ioBufFrames;
    }

    for (size_t i = 0; i < rem; ++i) {
        ioBuf[0] = *(buf0.data + frSrc);
        ioBuf[1] = *(buf1.data + frSrc);
        if (file.writef(ioBuf, 1) != 1) {
            std::cerr << "BufDiskWorker::writeBufferStereo(): write aborted (disk space?) after " << nf << " frames"
                      << std::endl;
            goto cleanup;
        }
        ++frSrc;
        ++nf;
    }
    cleanup:
    delete[] ioBuf;
}

void BufDiskWorker::render(BufDesc &buf, float start, float dur, size_t samples, RenderCallback callback) {
    if (samples == 0) { return; }

    size_t frStart = secToFrame(start);
    if (frStart < 0) { frStart = 0; }
    clamp(frStart, buf.frames - 1);

    size_t frDur;
    if (dur < 0) {
        frDur = buf.frames - frStart;
    } else {
        frDur = secToFrame(dur);
    }
    if (frDur < 1) { return; }
    clamp(frDur, buf.frames - frStart);
    clamp(samples, frDur);
    dur = frDur / (float)sampleRate;
    float window = (float)dur / samples;

    auto *sampleBuf = new float[samples];

    size_t m;
    if (frDur <= samples) {
        // no peak finding
        for (m = 0; m < samples; m++) {
            sampleBuf[m] = buf.data[frStart + m];
        }
    } else {
        size_t w, wStart, wEnd;
        float peak;

        // FIXME -- sloppy heuristic for how many frames to skip when peak finding
        int stride = (int)std::log2f(dur / 4);
        if (stride < 1) { stride = 1; }
        for (m = 1; m <= samples; m++) {
            wStart = secToFrame(start + (m - 1) * window);
            wEnd = secToFrame(start + m * window);
            peak = 0.f;
            for (w = wStart; w < wEnd; w += stride) {
                if (std::fabs(buf.data[w]) > std::fabs(peak)) {
                    peak = buf.data[w];
                }
            }
            sampleBuf[m - 1] = peak;
        }
    }

    callback(window, start, samples, sampleBuf);
    delete[] sampleBuf;
}

void BufDiskWorker::process(BufDesc &buf, float start, float dur, ProcessCallback processCallback) {
    size_t frStart = secToFrame(start);
    if (frStart > buf.frames - 1) { return; }

    size_t frDur;
    if (dur < 0) {
        frDur = buf.frames - frStart;
    } else {
        frDur = secToFrame(dur);
    }
    clamp(frDur, buf.frames - frStart);

    if  (fd == -1) { 
        std::cerr << "BufDiskWorker::process(): opening shared memory failed"  << std::endl;
        return;
    }
    size_t size = sizeof(float) * frDur;
    if (ftruncate(fd, size) == -1) {
        // can't process the whole buffer, so let's just give up
        std::cerr << "BufDiskWorker::process(): resizing shared memory failed"  << std::endl;
        return;
    }
    float *BufDiskWorker_shm = (float *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (BufDiskWorker_shm == MAP_FAILED) {
        // again, just give up
        std::cerr << "BufDiskWorker::process(): mapping shared memory failed"  << std::endl;
        return;
    }
    for (size_t i = 0; i < frDur; ++i) {
        BufDiskWorker_shm[i] = buf.data[frStart];
        frStart++;
    }
    processCallback(frDur);
}

void BufDiskWorker::poke(BufDesc &buf, float start, float dur, DoneCallback doneCallback) {
    size_t frDur = secToFrame(dur);
    size_t frStart = secToFrame(start);
    clamp(frDur, buf.frames - frStart);
    if (fd == -1) { 
        // just give up
        std::cerr << "BufDiskWorker::poke(): opening shared memory failed"  << std::endl;
        return; 
    }
    size_t size = sizeof(float) * frDur;
    float *BufDiskWorker_shm = (float *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (BufDiskWorker_shm == MAP_FAILED) {
        // just give up
        std::cerr << "BufDiskWorker::poke(): mapping shared memory failed"  << std::endl;
        return;
    }
    for (size_t i = 0; i < frDur; ++i) {
        buf.data[frStart] = BufDiskWorker_shm[i];
        frStart++;
    }
    std::cerr << "calling doneCallback" << std::endl;
    doneCallback(0);
}
