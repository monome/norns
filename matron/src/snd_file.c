
#include <sndfile.h>

#include "snd_file.h"

struct snd_file_desc snd_file_inspect(const char *path) {
    SF_INFO sfinfo = {.format = 0};
    SNDFILE *sndfile = sf_open(path, SFM_READ, &sfinfo);
    struct snd_file_desc res = {.channels = 0, .frames = 0, .samplerate = 0};
    if (sndfile == NULL) {
        return res;
    }
    res.channels = sfinfo.channels;
    res.frames = sfinfo.frames;
    res.samplerate = sfinfo.samplerate;
    return res;
}
