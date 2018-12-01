//
// Created by emb on 12/01/18.
//

#ifndef CRONE_TAPE_H
#define CRONE_TAPE_H

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <jack/types.h>
#include <jack/ringbuffer.h>
#include <sndfile.h>

template <int NumChannels>
class Tape {
public:
    class DiskAccess {
    private:
        SNDFILE *file;
        std::thread th;
        std::mutex lock;
        std::condition_variable cv;
	    jack_ringbuffer_t *buf;
        static constexpr size_t sampleSize= sizeof(jack_default_audio_sample_t);
    public:
        DiskAccess(int ringBufSize=2048) {
            buf = jack_ringbuffer_create(sampleSize * NumChannels * ringBufSize);
        }
    };
    
    class Writer: DiskAccess {
    public:
        void process(const float *src[NumChannels], size_t numFrames) {

        }
    };
    

    class Reader : DiskAccess {
        void process (float *src[NumChannels], size_t numFrames) {

        }
    };

    Writer writer;
    Reader reader;

public:

    
};

#endif // CRONE_TAPE_H
