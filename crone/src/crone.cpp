#include "crone.h"
#include "oracle.h"
#include "MixerClient.h"
#include "SoftcutClient.h"

static crone::MixerClient *mixerClient;
static crone::SoftcutClient *softcutClient;

void crone_init(crone::MixerClient *m, crone::SoftcutClient *sc) {
  mixerClient = m;
  softcutClient = sc;
}

void crone_poll_start_vu() {
  mixerClient->getVuPoll()->start();
}

void crone_poll_stop_vu() {
  mixerClient->getVuPoll()->stop();
}

void crone_cut_buffer_read_mono(const char *path, 
                                float startSrc=0.f, float startDst=0.f, float dur=-1.f, int chSrc=0, int chDst=0,
                                float preserve=0.f, float mix=1.f) {
  softcutClient->readBufferMono(path, startSrc, startDst, dur, chSrc, chDst,
                                preserve, mix);
}

void crone_cut_buffer_read_stereo(const char *path, 
                                  float startSrc = 0.f,
                                  float startDst = 0.f,
                                  float dur = -1.f,
                                  float preserve = 0.f,
                                  float mix = 1.f) {
  softcutClient->readBufferStereo(path, startSrc, startDst, dur, preserve, mix);
}

void crone_cut_buffer_write_mono(const char *path, 
                                float start = 0.f,
                                float dur = -1.f,
                                int chan = 0) {
  softcutClient->writeBufferMono(path, start, dur, chan);
}

void crone_cut_buffer_write_stereo(const char *path, 
                                  float start = 0.f,
                                  float dur = -1.f) {
  softcutClient->writeBufferStereo(path, start, dur);
}

void crone_cut_buffer_clear() {
  softcutClient->clearBuffer(0);
  softcutClient->clearBuffer(1);
}

void crone_cut_buffer_clear_channel(int ch=0) {
  softcutClient->clearBuffer(ch);
}

void crone_cut_buffer_clear_region(float start, float dur, float fadeTime,
                                   float preserve) {
  softcutClient->clearBufferWithFade(0, start, dur, fadeTime, preserve);
  softcutClient->clearBufferWithFade(1, start, dur, fadeTime, preserve);
}

void crone_cut_buffer_clear_region_channel(int ch, float start, float dur,
                                           float fadeTime, float preserve) {
  softcutClient->clearBufferWithFade(ch, start, dur, fadeTime, preserve);
}

void crone_cut_buffer_copy_mono(int srcCh=0, int dstCh=0, 
                            float srcStart=0, float dstStart=0,
                            float dur = -1.f,
                            float fadeTime = 0.f,
                            float preserve = 0.f,
                            bool reverse = false) {
  softcutClient->copyBuffer(srcCh, dstCh, srcStart, dstStart, dur, fadeTime, preserve,
                            reverse);
}

void crone_cut_buffer_copy_stereo(float srcStart=0, float dstStart=0,
                            float dur = -1.f,
                            float fadeTime = 0.f,
                            float preserve = 0.f,
                            bool reverse = false) {
  softcutClient->copyBuffer(0, 0, srcStart, dstStart, dur, fadeTime, preserve, reverse);
  softcutClient->copyBuffer(1, 1, srcStart, dstStart, dur, fadeTime, preserve, reverse);
}

void crone_cut_buffer_render(int ch, float start, float dur, int sampleCt = 128) {
  softcutClient->renderSamples(
      ch, start, dur, sampleCt,
      [=](float secPerSample, float start, size_t count, const float *samples) {
        o_poll_callback_softcut_render(ch, secPerSample, start, count, samples);
      }
  );
}

void crone_cut_query_position(int idx) {
  float pos = softcutClient->getPosition(idx);
  o_poll_callback_softcut_position(idx, pos);
}

void crone_cut_reset() {
  softcutClient->clearBuffer(0, 0, -1);
  softcutClient->clearBuffer(1, 0, -1);

  softcutClient->reset();
  for (int i = 0; i < crone::SoftcutClient::NumVoices; ++i) {
    softcutClient->getPhasePoll()->stop();
  }
}

void crone_set_param_cut_phase_quant(int arg0, float arg1) {
  softcutClient->setPhaseQuant(arg0, arg1);
}

void crone_set_param_cut_phase_offset(int arg0, float arg1) {
  softcutClient->setPhaseOffset(arg0, arg1);
}

void crone_poll_start_cut_phase() { softcutClient->getPhasePoll()->start(); }

void crone_poll_stop_cut_phase() { softcutClient->getPhasePoll()->stop(); }

void crone_tape_rec_open(const char *path) {
  mixerClient->openTapeRecord(path);
}

void crone_tape_rec_start() { mixerClient->startTapeRecord(); }

void crone_tape_rec_stop() { mixerClient->stopTapeRecord(); }

void crone_tape_play_open(const char *path) {
  mixerClient->openTapePlayback(path);
}

void crone_tape_play_start() { mixerClient->startTapePlayback(); }

void crone_tape_play_stop() { mixerClient->stopTapePlayback(); }
