// just some light glue to libsndfile, for inspecting files on disk

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct snd_file_desc {
    int channels;
    int frames;
    int samplerate;
};

extern struct snd_file_desc snd_file_inspect(const char *path);

#ifdef __cplusplus
}
#endif
