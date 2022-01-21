#include "crone.h"
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

void crone_cut_buffer_read_mono(const char *arg0, float arg1, float arg2,
                                float arg3, int arg4, int arg5, float arg6,
                                float arg7) {
  // FIXME: defaults
  float startSrc = 0.f;
  float startDst = 0.f;
  float dur = -1.f;
  int chanSrc = 0;
  int chanDst = 0;
  float preserve = 0.f;
  float mix = 1.f;
  softcutClient->readBufferMono(arg0, startSrc, startDst, dur, chanSrc, chanDst,
                                preserve, mix);
}

void crone_cut_buffer_read_stereo(const char *arg0, float arg1, float arg2,
                                  float arg3, float arg4, float arg5) {
  // FIXME: defaults
  // FIXME: default args
  float startSrc = 0.f;
  float startDst = 0.f;
  float dur = -1.f;
  float preserve = 0.f;
  float mix = 1.f;
  const char *str = arg0;
  softcutClient->readBufferStereo(str, startSrc, startDst, dur, preserve, mix);
}

void crone_cut_buffer_write_mono(const char *arg0, float arg1, float arg2,
                                 int arg3) {
  // FIXME: defaults
  float start = 0.f;
  float dur = -1.f;
  int chan = 0;
  const char *str = arg0;
  softcutClient->writeBufferMono(str, start, dur, chan);
}

void crone_cut_buffer_write_stereo(const char *arg0, float arg1, float arg2) {
  // FIXME: default args
  float start = 0.f;
  float dur = -1.f;
  const char *str = arg0;
  softcutClient->writeBufferStereo(str, start, dur);
}

void crone_cut_buffer_clear() {
  softcutClient->clearBuffer(0);
  softcutClient->clearBuffer(1);
}

void crone_cut_buffer_clear_channel(int arg0) {
  softcutClient->clearBuffer(arg0);
}

void crone_cut_buffer_clear_region(float start, float dur, float fade_time,
                                   float preserve) {
  softcutClient->clearBufferWithFade(0, start, dur, fade_time, preserve);
  softcutClient->clearBufferWithFade(1, start, dur, fade_time, preserve);
}

void crone_cut_buffer_clear_region_channel(int ch, float start, float dur,
                                           float fade_time, float preserve) {
  softcutClient->clearBufferWithFade(ch, start, dur, fade_time, preserve);
}

void crone_cut_buffer_clear_fade_region(float arg0, float arg1, float arg2,
                                        float arg3) {
  // FIXME: default args
  float dur = -1;
  float fadeTime = 0;
  float preserve = 0;
  softcutClient->clearBufferWithFade(0, arg0, dur, fadeTime, preserve);
  softcutClient->clearBufferWithFade(1, arg0, dur, fadeTime, preserve);
}

void crone_cut_buffer_clear_fade_region_channel(int arg0, float arg1,
                                                float arg2, float arg3,
                                                float arg4) {
  // FIXME: default args
  float dur = -1;
  float fadeTime = 0;
  float preserve = 0;
  softcutClient->clearBufferWithFade(arg0, arg1, dur, fadeTime, preserve);
}

void crone_cut_buffer_copy_mono(int arg0, int arg1, float arg2, float arg3,
                                float arg4, float arg5, float arg6, int arg7) {
  // FIXME: default args
  float dur = -1.f;
  float fadeTime = 0.f;
  float preserve = 0.f;
  bool reverse = false;

  softcutClient->copyBuffer(arg0, arg1, arg2, arg3, dur, fadeTime, preserve,
                            reverse);
}

void crone_cut_buffer_copy_stereo(float arg0, float arg1, float arg2,
                                  float arg3, float arg4, int arg5) {
  // FIXME: default args
  float dur = -1.f;
  float fadeTime = 0.f;
  float preserve = 0.f;
  bool reverse = false;

  softcutClient->copyBuffer(0, 0, arg0, arg1, dur, fadeTime, preserve, reverse);
  softcutClient->copyBuffer(1, 1, arg0, arg1, dur, fadeTime, preserve, reverse);
}

void crone_cut_buffer_render(int arg0, float arg1, float arg2, int arg3) {
  int sampleCt = 128;
  int ch = arg0;

  softcutClient->renderSamples(
      ch, arg1, arg2, sampleCt,
      [=](float secPerSample, float start, size_t count, float *samples) {
        // FIXME: perform render callback directly.. maybe take arg to FP

        // lo_blob bl = lo_blob_new(count * sizeof(float), samples);
        // lo_send(matronAddress, "/softcut/buffer/render_callback", "iffb", ch,
        //         secPerSample, start, bl);
      });
}

void crone_cut_query_position(int arg0) {
  int idx = arg0;
  float pos = softcutClient->getPosition(idx);
  (void)pos;
  // FIXME
  // lo_send(matronAddress, "/poll/softcut/position", "if", idx, pos);
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

void crone_tape_rec_open(const char *arg0) {
  mixerClient->openTapeRecord(arg0);
}

void crone_tape_rec_start() { mixerClient->startTapeRecord(); }

void crone_tape_rec_stop() { mixerClient->stopTapeRecord(); }

void crone_tape_play_open(const char *arg0) {
  mixerClient->openTapePlayback(arg0);
}

void crone_tape_play_start() { mixerClient->startTapePlayback(); }

void crone_tape_play_stop() { mixerClient->stopTapePlayback(); }
