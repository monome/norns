#include <functional>
#include <map>
#include <stdexcept>

#include "crone.h"
#include "crone_param_dispatch.h"

std::map<const std::string, std::function<void(int, float)>> cut_param_fn_map =
    {{"play_flag", crone_set_param_cut_play_flag},
     {"rate", crone_set_param_cut_rate},
     {"loop_start", crone_set_param_cut_loop_start},
     {"loop_end", crone_set_param_cut_loop_end},
     {"loop_flag", crone_set_param_cut_loop_flag},
     {"fade_time", crone_set_param_cut_fade_time},
     {"rec_level", crone_set_param_cut_rec_level},
     {"pre_level", crone_set_param_cut_pre_level},
     {"rec_flag", crone_set_param_cut_rec_flag},
     {"rec_offset", crone_set_param_cut_rec_offset},
     {"position", crone_set_param_cut_position},
     {"pre_filter_fc", crone_set_param_cut_pre_filter_fc},
     {"pre_filter_fc_mod", crone_set_param_cut_pre_filter_fc_mod},
     {"pre_filter_rq", crone_set_param_cut_pre_filter_rq},
     {"pre_filter_lp", crone_set_param_cut_pre_filter_lp},
     {"pre_filter_hp", crone_set_param_cut_pre_filter_hp},
     {"pre_filter_bp", crone_set_param_cut_pre_filter_bp},
     {"pre_filter_br", crone_set_param_cut_pre_filter_br},
     {"pre_filter_dry", crone_set_param_cut_pre_filter_dry},
     {"post_filter_fc", crone_set_param_cut_post_filter_fc},
     {"post_filter_rq", crone_set_param_cut_post_filter_rq},
     {"post_filter_lp", crone_set_param_cut_post_filter_lp},
     {"post_filter_hp", crone_set_param_cut_post_filter_hp},
     {"post_filter_bp", crone_set_param_cut_post_filter_bp},
     {"post_filter_br", crone_set_param_cut_post_filter_br},
     {"post_filter_dry", crone_set_param_cut_post_filter_dry},
     {"level_slew_time", crone_set_param_cut_level_slew_time},
     {"pan_slew_time", crone_set_param_cut_pan_slew_time},
     {"recpre_slew_time", crone_set_param_cut_recpre_slew_time},
     {"rate_slew_time", crone_set_param_cut_rate_slew_time}
};

std::map<const std::string, std::function<void(int, int)>> cut_param_fn_map_ii =
    {
        {"buffer", crone_set_param_cut_buffer},
};

std::map<const std::string, std::function<void(int, int, float)>>
    cut_param_fn_map_iif = {
        {"voice_sync", crone_set_param_cut_voice_sync},
};

std::map<const std::string, std::function<void(float)>>
reverb_param_fn_map = {
  {"hf_damp", crone_set_param_reverb_hf_damp},
  {"mid_rt60", crone_set_param_reverb_mid_rt60},
  {"low_rt60", crone_set_param_reverb_low_rt60},
  {"lf_fc", crone_set_param_reverb_lf_fc},
  {"pre_del", crone_set_param_reverb_pre_del},
};

std::map<const std::string, std::function<void(float)>>
compressor_param_fn_map = {
  {"ratio", crone_set_param_compressor_ratio},
  {"threshold", crone_set_param_compressor_threshold},
  {"attack", crone_set_param_compressor_attack},
  {"release", crone_set_param_compressor_release},
  {"gain_pre", crone_set_param_compressor_gain_pre},
  {"gain_post", crone_set_param_compressor_gain_post},
  
};

void crone_set_cut_param(const char* name, int voice, float value) {
  const std::string k(name);
  try {
    auto fn = cut_param_fn_map.at(k);
    fn(voice, value);
  } catch (std::out_of_range& ex) {
    std::cerr << "unknown softcut parameter:" << name << std::endl;
  }
}

void crone_set_cut_param_ii(const char* name, int voice, int value) {
  const std::string k(name);
  try {
    auto fn = cut_param_fn_map_ii.at(k);
    fn(voice, value);
  } catch (std::out_of_range& ex) {
    std::cerr << "unknown softcut parameter:" << name << std::endl;
  }
}

void crone_set_cut_param_iif(const char* name, int a, int b, float value) {
  const std::string k(name);
  try {
    auto fn = cut_param_fn_map_iif.at(k);
    fn(a, b, value);
  } catch (std::out_of_range& ex) {
    std::cerr << "unknown softcut parameter:" << name << std::endl;
  }
}

void crone_set_reverb_param(const char* name, float value) {
  const std::string k(name);
  try {
    auto fn = reverb_param_fn_map.at(k);
    fn(value);
  } catch (std::out_of_range& ex) {
    std::cerr << "unknown reverb parameter:" << name << std::endl;
  }
}

void crone_set_compressor_param(const char* name, float value) {
  const std::string k(name);
  try {
    auto fn = compressor_param_fn_map.at(k);
    fn(value);
  } catch (std::out_of_range& ex) {
    std::cerr << "unknown compressor parameter:" << name << std::endl;
  }
}
