#ifndef _CRONE_H_
#define _CRONE_H_

#include <iostream>

#include "BufDiskWorker.h"
#include "Commands.h"
#include "MixerClient.h"
#include "Poll.h"
#include "SoftcutClient.h"
#include "effects/CompressorParams.h"
#include "effects/ReverbParams.h"
#include "softcut/FadeCurves.h"

void crone_init(crone::MixerClient *mixerClient,
                crone::SoftcutClient *softCutClient);

void crone_poll_start_vu();

void crone_poll_stop_vu();

//////////////////
/// FIXME: rewrite signatures with useful parameter names / defaults!
/// (signatures in definition are revised; copy them with a script/macro)

inline void crone_set_level_adc(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_ADC, arg0);
}

inline void crone_set_level_dac(float arg0) {
  std::cerr << "crone_set_level_dac(" << arg0 << ")" << std::endl;
  std::cerr << "&mixerCommands: " << std::hex
            << &(crone::Commands::mixerCommands) << std::endl;
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_DAC, arg0);
}

inline void crone_set_level_ext(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_EXT, arg0);
}

inline void crone_set_level_cut_master(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_CUT_MASTER,
                                      arg0);
}

inline void crone_set_level_ext_rev(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_EXT_AUX,
                                      arg0);
}

inline void crone_set_level_rev_dac(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_AUX_DAC,
                                      arg0);
}

inline void crone_set_level_monitor(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_MONITOR,
                                      arg0);
}

inline void crone_set_level_monitor_mix(int arg0, float arg1) {
  crone::Commands::mixerCommands.post(
      crone::Commands::Id::SET_LEVEL_MONITOR_MIX, arg0, arg1);
}

inline void crone_set_level_monitor_rev(float arg0) {
  crone::Commands::mixerCommands.post(
      crone::Commands::Id::SET_LEVEL_MONITOR_AUX, arg0);
}

inline void crone_set_level_compressor_mix(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_INS_MIX,
                                      arg0);
}

inline void crone_set_enabled_compressor(float arg0) {
  crone::Commands::mixerCommands.post(
      crone::Commands::Id::SET_ENABLED_COMPRESSOR, arg0);
}

inline void crone_set_enabled_reverb(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_ENABLED_REVERB,
                                      arg0);
}

inline void crone_set_param_compressor_ratio(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_COMPRESSOR,
                                      crone::CompressorParam::RATIO, arg0);
}

inline void crone_set_param_compressor_threshold(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_COMPRESSOR,
                                      crone::CompressorParam::THRESHOLD, arg0);
}

inline void crone_set_param_compressor_attack(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_COMPRESSOR,
                                      crone::CompressorParam::ATTACK, arg0);
}

inline void crone_set_param_compressor_release(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_COMPRESSOR,
                                      crone::CompressorParam::RELEASE, arg0);
}

inline void crone_set_param_compressor_gain_pre(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_COMPRESSOR,
                                      crone::CompressorParam::GAIN_PRE, arg0);
}

inline void crone_set_param_compressor_gain_post(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_COMPRESSOR,
                                      crone::CompressorParam::GAIN_POST, arg0);
}

inline void crone_set_param_reverb_pre_del(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_REVERB,
                                      crone::ReverbParam::PRE_DEL, arg0);
}

inline void crone_set_param_reverb_lf_fc(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_REVERB,
                                      crone::ReverbParam::LF_FC, arg0);
}

inline void crone_set_param_reverb_low_rt60(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_REVERB,
                                      crone::ReverbParam::LOW_RT60, arg0);
}

inline void crone_set_param_reverb_mid_rt60(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_REVERB,
                                      crone::ReverbParam::MID_RT60, arg0);
}

inline void crone_set_param_reverb_hf_damp(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_PARAM_REVERB,
                                      crone::ReverbParam::HF_DAMP, arg0);
}

inline void crone_set_enabled_cut(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_ENABLED_CUT,
                                        arg0, arg1);
}

inline void crone_set_level_cut(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_LEVEL_CUT,
                                        arg0, arg1);
}

inline void crone_set_pan_cut(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_PAN_CUT, arg0,
                                        arg1);
}

inline void crone_set_level_adc_cut(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_ADC_CUT,
                                      arg0);
}

inline void crone_set_level_ext_cut(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_EXT_CUT,
                                      arg0);
}

inline void crone_set_level_tape_cut(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_TAPE_CUT,
                                      arg0);
}

inline void crone_set_level_cut_rev(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_CUT_AUX,
                                      arg0);
}

inline void crone_set_level_in_cut(int arg0, int arg1, float arg2) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_LEVEL_IN_CUT,
                                        arg0, arg1, arg2);
}

inline void crone_set_level_cut_cut(int arg0, int arg1, float arg2) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_LEVEL_CUT_CUT,
                                        arg0, arg1, arg2);
}

inline void crone_set_param_cut_rate(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_RATE, arg0,
                                        arg1);
}

inline void crone_set_param_cut_loop_start(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_LOOP_START,
                                        arg0, arg1);
}

inline void crone_set_param_cut_loop_end(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_LOOP_END,
                                        arg0, arg1);
}

inline void crone_set_param_cut_loop_flag(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_LOOP_FLAG,
                                        arg0, arg1);
}

inline void crone_set_param_cut_fade_time(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_FADE_TIME,
                                        arg0, arg1);
}

inline void crone_set_param_cut_rec_level(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_REC_LEVEL,
                                        arg0, arg1);
}

inline void crone_set_param_cut_pre_level(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_PRE_LEVEL,
                                        arg0, arg1);
}

inline void crone_set_param_cut_rec_flag(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_REC_FLAG,
                                        arg0, arg1);
}

inline void crone_set_param_cut_play_flag(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_PLAY_FLAG,
                                        arg0, arg1);
}

inline void crone_set_param_cut_rec_offset(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_REC_OFFSET,
                                        arg0, arg1);
}

inline void crone_set_param_cut_position(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_POSITION,
                                        arg0, arg1);
}

inline void crone_set_param_cut_pre_filter_fc(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_PRE_FILTER_FC, arg0, arg1);
}

inline void crone_set_param_cut_pre_filter_fc_mod(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_PRE_FILTER_FC_MOD, arg0, arg1);
}

inline void crone_set_param_cut_pre_filter_rq(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_PRE_FILTER_RQ, arg0, arg1);
}

inline void crone_set_param_cut_pre_filter_lp(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_PRE_FILTER_LP, arg0, arg1);
}

inline void crone_set_param_cut_pre_filter_hp(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_PRE_FILTER_HP, arg0, arg1);
}

inline void crone_set_param_cut_pre_filter_bp(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_PRE_FILTER_BP, arg0, arg1);
}

inline void crone_set_param_cut_pre_filter_br(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_PRE_FILTER_BR, arg0, arg1);
}

inline void crone_set_param_cut_pre_filter_dry(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_PRE_FILTER_DRY, arg0, arg1);
}

inline void crone_set_param_cut_post_filter_fc(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_POST_FILTER_FC, arg0, arg1);
}

inline void crone_set_param_cut_post_filter_rq(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_POST_FILTER_RQ, arg0, arg1);
}

inline void crone_set_param_cut_post_filter_lp(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_POST_FILTER_LP, arg0, arg1);
}

inline void crone_set_param_cut_post_filter_hp(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_POST_FILTER_HP, arg0, arg1);
}

inline void crone_set_param_cut_post_filter_bp(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_POST_FILTER_BP, arg0, arg1);
}

inline void crone_set_param_cut_post_filter_br(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_POST_FILTER_BR, arg0, arg1);
}

inline void crone_set_param_cut_post_filter_dry(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_POST_FILTER_DRY, arg0, arg1);
}

inline void crone_set_param_cut_voice_sync(int arg0, int arg1, float arg2) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_VOICE_SYNC,
                                        arg0, arg1, arg2);
}

inline void crone_set_param_cut_pre_fade_window(int arg0, float arg1) {
  (void)arg0;
  (void)arg1;
  // FIXME: revise fade shapes
}

inline void crone_set_param_cut_rec_fade_delay(int arg0, float arg1) {
  (void)arg0;
  (void)arg1;
  // FIXME: revise fade shapes
}

inline void crone_set_param_cut_pre_fade_shape(int arg0, float arg1) {
  (void)arg0;
  (void)arg1;
  // FIXME: revise fade shapes
}

inline void crone_set_param_cut_rec_fade_shape(int arg0, float arg1) {
  (void)arg0;
  (void)arg1;
  // FIXME: revise fade shapes
}

inline void crone_set_param_cut_level_slew_time(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_LEVEL_SLEW_TIME, arg0, arg1);
}

inline void crone_set_param_cut_pan_slew_time(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_PAN_SLEW_TIME, arg0, arg1);
}

inline void crone_set_param_cut_recpre_slew_time(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_RECPRE_SLEW_TIME, arg0, arg1);
}

inline void crone_set_param_cut_rate_slew_time(int arg0, float arg1) {
  crone::Commands::softcutCommands.post(
      crone::Commands::Id::SET_CUT_RATE_SLEW_TIME, arg0, arg1);
}

inline void crone_set_param_cut_buffer(int arg0, int arg1) {
  crone::Commands::softcutCommands.post(crone::Commands::Id::SET_CUT_BUFFER,
                                        arg0, arg1);
}

void crone_cut_buffer_read_mono(const char *arg0, float arg1, float arg2,
                                float arg3, int arg4, int arg5, float arg6,
                                float arg7);
void crone_cut_buffer_read_stereo(const char *arg0, float arg1, float arg2,
                                  float arg3, float arg4, float arg5);
void crone_cut_buffer_write_mono(const char *arg0, float arg1, float arg2,
                                 int arg3);

void crone_cut_buffer_write_stereo(const char *arg0, float arg1, float arg2);

void crone_cut_buffer_clear();
void crone_cut_buffer_clear_channel(int arg0);
void crone_cut_buffer_clear_region(float start, float dur, float fade_time,
                                   float preserve);
void crone_cut_buffer_clear_region_channel(int ch, float start, float dur,
                                           float fade_time, float preserve);
void crone_cut_buffer_clear_fade_region(float arg0, float arg1, float arg2,
                                        float arg3);
void crone_cut_buffer_clear_fade_region_channel(int arg0, float arg1,
                                                float arg2, float arg3,
                                                float arg4);
void crone_cut_buffer_copy_mono(int arg0, int arg1, float arg2, float arg3,
                                float arg4, float arg5, float arg6, bool arg7);
void crone_cut_buffer_copy_stereo(float arg0, float arg1, float arg2,
                                  float arg3, float arg4, bool arg5);
void crone_cut_buffer_render(int arg0, float arg1, float arg2, int arg3);
void crone_cut_query_position(int arg0);

void crone_cut_reset();
void crone_set_param_cut_phase_quant(int arg0, float arg1);
void crone_set_param_cut_phase_offset(int arg0, float arg1);
void crone_poll_start_cut_phase();
void crone_poll_stop_cut_phase();
void crone_tape_rec_open(const char *arg0);
void crone_tape_rec_start();
void crone_tape_rec_stop();
void crone_tape_play_open(const char *arg0);
void crone_tape_play_start();
void crone_tape_play_stop();

inline void crone_set_level_tape(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_TAPE,
                                      arg0);
}

inline void crone_set_level_tape_rev(float arg0) {
  crone::Commands::mixerCommands.post(crone::Commands::Id::SET_LEVEL_TAPE_AUX,
                                      arg0);
}

#endif