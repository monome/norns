--- Molly the Poly lib
-- Engine params and functions.
--
-- @module MollyThePoly
-- @release v1.0.0
-- @author Mark Eats

local ControlSpec = require "controlspec"
local Formatters = require "jah/formatters"

local MollyThePoly = {}

local specs = {}
local options = {}

options.OSC_WAVE_SHAPE = {"Triangle", "Saw", "Pulse"}
specs.PW_MOD = ControlSpec.new(0, 1, "lin", 0, 0.2, "")
options.PW_MOD_SRC = {"LFO", "Env 1", "Manual"}

specs.FREQ_MOD_LFO = ControlSpec.UNIPOLAR
specs.FREQ_MOD_ENV = ControlSpec.BIPOLAR
specs.GLIDE = ControlSpec.new(0, 5, "lin", 0, 0, "s")

specs.MAIN_OSC_LEVEL = ControlSpec.new(0, 1, "lin", 0, 1, "")
specs.SUB_OSC_LEVEL = ControlSpec.UNIPOLAR
specs.SUB_OSC_DETUNE = ControlSpec.new(-5, 5, "lin", 0, 0, "ST")
specs.NOISE_LEVEL = ControlSpec.new(0, 1, "lin", 0, 0.1, "")

specs.HP_FILTER_CUTOFF = ControlSpec.new(10, 20000, "exp", 0, 10, "Hz")
specs.LP_FILTER_CUTOFF = ControlSpec.new(20, 20000, "exp", 0, 300, "Hz")
specs.LP_FILTER_RESONANCE = ControlSpec.new(0, 1, "lin", 0, 0.1, "")
options.LP_FILTER_TYPE = {"-12 dB/oct", "-24 dB/oct"}
options.LP_FILTER_ENV = {"Env-1", "Env-2"}
specs.LP_FILTER_CUTOFF_MOD_ENV = ControlSpec.new(-1, 1, "lin", 0, 0.25, "")
specs.LP_FILTER_CUTOFF_MOD_LFO = ControlSpec.UNIPOLAR
specs.LP_FILTER_TRACKING = ControlSpec.new(0, 2, "lin", 0, 1, ":1")

specs.LFO_FREQ = ControlSpec.new(0.05, 20, "exp", 0, 5, "Hz")
options.LFO_WAVE_SHAPE = {"Sine", "Triangle", "Saw", "Square", "Random"}
specs.LFO_FADE = ControlSpec.new(-15, 15, "lin", 0, 0, "s")

specs.ENV_ATTACK = ControlSpec.new(0.002, 5, "lin", 0, 0.01, "s")
specs.ENV_DECAY = ControlSpec.new(0.002, 10, "lin", 0, 0.3, "s")
specs.ENV_SUSTAIN = ControlSpec.new(0, 1, "lin", 0, 0.5, "")
specs.ENV_RELEASE = ControlSpec.new(0.002, 10, "lin", 0, 0.5, "s")

specs.AMP = ControlSpec.new(0, 11, "lin", 0, 0.5, "")
specs.AMP_MOD = ControlSpec.UNIPOLAR

specs.RING_MOD_FREQ = ControlSpec.new(10, 300, "exp", 0, 50, "Hz")
specs.RING_MOD_FADE = ControlSpec.new(-15, 15, "lin", 0, 0, "s")
specs.RING_MOD_MIX = ControlSpec.UNIPOLAR

specs.CHORUS_MIX = ControlSpec.new(0, 1, "lin", 0, 0.8, "")

MollyThePoly.specs = specs


local function format_ratio_to_one(param)
  return util.round(param:get(), 0.01) .. ":1"
end

local function format_fade(param)
  local secs = util.round(param:get(), 0.01)
  local suffix = " in"
  if secs < 0 then
    secs = secs - specs.LFO_FADE.minval
    suffix = " out"
  end
  return math.abs(secs) .. " s" .. suffix
end

function MollyThePoly.add_params()
  
  params:add{type = "option", id = "osc_wave_shape", name = "Osc Wave Shape", options = options.OSC_WAVE_SHAPE, default = 3, action = function(value) engine.oscWaveShape(value - 1) end}
  params:add{type = "control", id = "pulse_width_mod", name = "Pulse Width Mod", controlspec = specs.PW_MOD, action = engine.pwMod}
  params:add{type = "option", id = "pulse_width_mod_src", name = "Pulse Width Mod Src", options = options.PW_MOD_SRC, action = function(value) engine.pwModSource(value - 1) end}
  params:add{type = "control", id = "freq_mod_lfo", name = "Frequency Mod (LFO)", controlspec = specs.FREQ_MOD_LFO, action = engine.freqModLfo}
  params:add{type = "control", id = "freq_mod_env", name = "Frequency Mod (Env-1)", controlspec = specs.FREQ_MOD_ENV, action = engine.freqModEnv}
  params:add{type = "control", id = "glide", name = "Glide", controlspec = specs.GLIDE, formatter = Formatters.format_secs, action = engine.glide}
  params:add_separator()
  
  params:add{type = "control", id = "main_osc_level", name = "Main Osc Level", controlspec = specs.MAIN_OSC_LEVEL, action = engine.mainOscLevel}
  params:add{type = "control", id = "sub_osc_level", name = "Sub Osc Level", controlspec = specs.SUB_OSC_LEVEL, action = engine.subOscLevel}
  params:add{type = "control", id = "sub_osc_detune", name = "Sub Osc Detune", controlspec = specs.SUB_OSC_DETUNE, action = engine.subOscDetune}
  params:add{type = "control", id = "noise_level", name = "Noise Level", controlspec = specs.NOISE_LEVEL, action = engine.noiseLevel}
  params:add_separator()
  
  params:add{type = "control", id = "hp_filter_cutoff", name = "HP Filter Cutoff", controlspec = specs.HP_FILTER_CUTOFF, formatter = Formatters.format_freq, action = engine.hpFilterCutoff}
  params:add{type = "control", id = "lp_filter_cutoff", name = "LP Filter Cutoff", controlspec = specs.LP_FILTER_CUTOFF, formatter = Formatters.format_freq, action = engine.lpFilterCutoff}
  params:add{type = "control", id = "lp_filter_resonance", name = "LP Filter Resonance", controlspec = specs.LP_FILTER_RESONANCE, action = engine.lpFilterResonance}
  params:add{type = "option", id = "lp_filter_type", name = "LP Filter Type", options = options.LP_FILTER_TYPE, default = 2, action = function(value) engine.lpFilterType(value - 1) end}
  params:add{type = "option", id = "lp_filter_env", name = "LP Filter Env", options = options.LP_FILTER_ENV, action = function(value) engine.lpFilterCutoffEnvSelect(value - 1) end}
  params:add{type = "control", id = "lp_filter_mod_env", name = "LP Filter Mod (Env)", controlspec = specs.LP_FILTER_CUTOFF_MOD_ENV, action = engine.lpFilterCutoffModEnv}
  params:add{type = "control", id = "lp_filter_mod_lfo", name = "LP Filter Mod (LFO)", controlspec = specs.LP_FILTER_CUTOFF_MOD_LFO, action = engine.lpFilterCutoffModLfo}
  params:add{type = "control", id = "lp_filter_tracking", name = "LP Filter Tracking", controlspec = specs.LP_FILTER_TRACKING, formatter = format_ratio_to_one, action = engine.lpFilterTracking}
  params:add_separator()
  
  params:add{type = "control", id = "lfo_freq", name = "LFO Frequency", controlspec = specs.LFO_FREQ, formatter = Formatters.format_freq, action = engine.lfoFreq}
  params:add{type = "option", id = "lfo_wave_shape", name = "LFO Wave Shape", options = options.LFO_WAVE_SHAPE, action = function(value) engine.lfoWaveShape(value - 1) end}
  params:add{type = "control", id = "lfo_fade", name = "LFO Fade", controlspec = specs.LFO_FADE, formatter = format_fade, action = function(value)
    if value < 0 then value = specs.LFO_FADE.minval - 0.00001 + math.abs(value) end
    engine.lfoFade(value)
  end}
  params:add_separator()
  
  params:add{type = "control", id = "env_1_attack", name = "Env-1 Attack", controlspec = specs.ENV_ATTACK, formatter = Formatters.format_secs, action = engine.env1Attack}
  params:add{type = "control", id = "env_1_decay", name = "Env-1 Decay", controlspec = specs.ENV_DECAY, formatter = Formatters.format_secs, action = engine.env1Decay}
  params:add{type = "control", id = "env_1_sustain", name = "Env-1 Sustain", controlspec = specs.ENV_SUSTAIN, action = engine.env1Sustain}
  params:add{type = "control", id = "env_1_release", name = "Env-1 Release", controlspec = specs.ENV_RELEASE, formatter = Formatters.format_secs, action = engine.env1Release}
  params:add_separator()
  
  params:add{type = "control", id = "env_2_attack", name = "Env-2 Attack", controlspec = specs.ENV_ATTACK, formatter = Formatters.format_secs, action = engine.env2Attack}
  params:add{type = "control", id = "env_2_decay", name = "Env-2 Decay", controlspec = specs.ENV_DECAY, formatter = Formatters.format_secs, action = engine.env2Decay}
  params:add{type = "control", id = "env_2_sustain", name = "Env-2 Sustain", controlspec = specs.ENV_SUSTAIN, action = engine.env2Sustain}
  params:add{type = "control", id = "env_2_release", name = "Env-2 Release", controlspec = specs.ENV_RELEASE, formatter = Formatters.format_secs, action = engine.env2Release}
  params:add_separator()
  
  params:add{type = "control", id = "amp", name = "Amp", controlspec = specs.AMP, action = engine.amp}
  params:add{type = "control", id = "amp_mod", name = "Amp Mod (LFO)", controlspec = specs.AMP_MOD, action = engine.ampMod}
  params:add_separator()
  
  params:add{type = "control", id = "ring_mod_freq", name = "Ring Mod Frequency", controlspec = specs.RING_MOD_FREQ, formatter = Formatters.format_freq, action = engine.ringModFreq}
  params:add{type = "control", id = "ring_mod_fade", name = "Ring Mod Fade", controlspec = specs.RING_MOD_FADE, formatter = format_fade, action = function(value)
    if value < 0 then value = specs.RING_MOD_FADE.minval - 0.00001 + math.abs(value) end
    engine.ringModFade(value)
  end}
  params:add{type = "control", id = "ring_mod_mix", name = "Ring Mod Mix", controlspec = specs.RING_MOD_MIX, action = engine.ringModMix}
  params:add{type = "control", id = "chorus_mix", name = "Chorus Mix", controlspec = specs.CHORUS_MIX, action = engine.chorusMix}
  params:add_separator()
  
  params:bang()
  
  params:add{type = "trigger", id = "create_lead", name = "Create Lead", action = function() MollyThePoly.randomize_params("lead") end}
  params:add{type = "trigger", id = "create_pad", name = "Create Pad", action = function() MollyThePoly.randomize_params("pad") end}
  params:add{type = "trigger", id = "create_percussion", name = "Create Percussion", action = function() MollyThePoly.randomize_params("percussion") end}
  
end

function MollyThePoly.randomize_params(sound_type)
  
  params:set("osc_wave_shape", math.random(#options.OSC_WAVE_SHAPE))
  params:set("pulse_width_mod", math.random())
  params:set("pulse_width_mod_src", math.random(#options.PW_MOD_SRC))
  
  params:set("lp_filter_type", math.random(#options.LP_FILTER_TYPE))
  params:set("lp_filter_env", math.random(#options.LP_FILTER_ENV))
  params:set("lp_filter_tracking", util.linlin(0, 1, specs.LP_FILTER_TRACKING.minval, specs.LP_FILTER_TRACKING.maxval, math.random()))
  
  params:set("lfo_freq", util.linlin(0, 1, specs.LFO_FREQ.minval, specs.LFO_FREQ.maxval, math.random()))
  params:set("lfo_wave_shape", math.random(#options.LFO_WAVE_SHAPE))
  params:set("lfo_fade", util.linlin(0, 1, specs.LFO_FADE.minval, specs.LFO_FADE.maxval, math.random()))
  
  params:set("env_1_decay", util.linlin(0, 1, specs.ENV_DECAY.minval, specs.ENV_DECAY.maxval, math.random()))
  params:set("env_1_sustain", math.random())
  params:set("env_1_release", util.linlin(0, 1, specs.ENV_RELEASE.minval, specs.ENV_RELEASE.maxval, math.random()))
  
  params:set("ring_mod_freq", util.linlin(0, 1, specs.RING_MOD_FREQ.minval, specs.RING_MOD_FREQ.maxval, math.random()))
  params:set("chorus_mix", math.random())
  
  
  if sound_type == "lead" then
    
    params:set("freq_mod_lfo", util.linexp(0, 1, 0.0000001, 0.1, math.pow(math.random(), 2)))
    if math.random() > 0.95 then
      params:set("freq_mod_env", util.linlin(0, 1, -0.06, 0.06, math.random()))
    else
      params:set("freq_mod_env", 0)
    end
    
    params:set("glide", util.linexp(0, 1, 0.0000001, 1, math.pow(math.random(), 2)))
    
    if math.random() > 0.8 then
      params:set("main_osc_level", 1)
      params:set("sub_osc_level", 0)
    else
      params:set("main_osc_level", math.random())
      params:set("sub_osc_level", math.random())
    end
    if math.random() > 0.9 then
      params:set("sub_osc_detune", util.linlin(0, 1, specs.SUB_OSC_DETUNE.minval, specs.SUB_OSC_DETUNE.maxval, math.random()))
    else
      local detune = {0, 0, 0, 4, 5, -4, -5}
      params:set("sub_osc_detune", detune[math.random(1, #detune)] + math.random() * 0.01)
    end
    params:set("noise_level", util.linexp(0, 1, 0.0000001, 1, math.random()))
    
    if math.abs(params:get("sub_osc_detune")) > 0.7 and params:get("sub_osc_level") > params:get("main_osc_level")  and params:get("sub_osc_level") > params:get("noise_level") then
      params:set("main_osc_level", params:get("sub_osc_level") + 0.2)
    end
    
    params:set("lp_filter_cutoff", util.linexp(0, 1, 100, specs.LP_FILTER_CUTOFF.maxval, math.pow(math.random(), 2)))
    params:set("lp_filter_resonance", math.random() * 0.9)
    params:set("lp_filter_mod_env", util.linlin(0, 1, math.random(-1, 0), 1, math.random()))
    params:set("lp_filter_mod_lfo", math.random() * 0.2)
    
    params:set("env_2_attack", util.linexp(0, 1, specs.ENV_ATTACK.minval, 0.5, math.random()))
    params:set("env_2_decay", util.linlin(0, 1, specs.ENV_DECAY.minval, specs.ENV_DECAY.maxval, math.random()))
    params:set("env_2_sustain", math.random())
    params:set("env_2_release", util.linlin(0, 1, specs.ENV_RELEASE.minval, 3, math.random()))
    
    if(math.random() > 0.8) then
      params:set("env_1_attack", params:get("env_2_attack"))
    else
      params:set("env_1_attack", util.linlin(0, 1, specs.ENV_ATTACK.minval, 1, math.random()))
    end
    
    if params:get("env_2_decay") < 0.2 and params:get("env_2_sustain") < 0.15 then
      params:set("env_2_decay", util.linlin(0, 1, 0.2, specs.ENV_DECAY.maxval, math.random()))
    end
    
    local amp_max = 0.9
    if math.random() > 0.8 then amp_max = 11 end
    params:set("amp", util.linlin(0, 1, 0.75, amp_max, math.random()))
    params:set("amp_mod", util.linlin(0, 1, 0, 0.5, math.random()))
    
    params:set("ring_mod_fade", util.linlin(0, 1, specs.RING_MOD_FADE.minval * 0.8, specs.RING_MOD_FADE.maxval * 0.3, math.random()))
    if(math.random() > 0.8) then
      params:set("ring_mod_mix", math.pow(math.random(), 2))
    else
      params:set("ring_mod_mix", 0)
    end
    
    
  elseif sound_type == "pad" then
    
    params:set("freq_mod_lfo", util.linexp(0, 1, 0.0000001, 0.2, math.pow(math.random(), 4)))
    if math.random() > 0.8 then
      params:set("freq_mod_env", util.linlin(0, 1, -0.1, 0.2, math.pow(math.random(), 4)))
    else
      params:set("freq_mod_env", 0)
    end
    
    params:set("glide", util.linexp(0, 1, 0.0000001, specs.GLIDE.maxval, math.pow(math.random(), 2)))
    
    params:set("main_osc_level", math.random())
    params:set("sub_osc_level", math.random())
    if math.random() > 0.7 then
      params:set("sub_osc_detune", util.linlin(0, 1, specs.SUB_OSC_DETUNE.minval, specs.SUB_OSC_DETUNE.maxval, math.random()))
    else
      params:set("sub_osc_detune", math.random(specs.SUB_OSC_DETUNE.minval, specs.SUB_OSC_DETUNE.maxval) + math.random() * 0.01)
    end
    params:set("noise_level", util.linexp(0, 1, 0.0000001, 1, math.random()))
    
    if math.abs(params:get("sub_osc_detune")) > 0.7 and params:get("sub_osc_level") > params:get("main_osc_level")  and params:get("sub_osc_level") > params:get("noise_level") then
      params:set("main_osc_level", params:get("sub_osc_level") + 0.2)
    end
    
    params:set("lp_filter_cutoff", util.linexp(0, 1, 100, specs.LP_FILTER_CUTOFF.maxval, math.random()))
    params:set("lp_filter_resonance", math.random())
    params:set("lp_filter_mod_env", util.linlin(0, 1, -1, 1, math.random()))
    params:set("lp_filter_mod_lfo", math.random())
    
    params:set("env_1_attack", util.linlin(0, 1, specs.ENV_ATTACK.minval, specs.ENV_ATTACK.maxval, math.random()))
    
    params:set("env_2_attack", util.linlin(0, 1, specs.ENV_ATTACK.minval, specs.ENV_ATTACK.maxval, math.random()))
    params:set("env_2_decay", util.linlin(0, 1, specs.ENV_DECAY.minval, specs.ENV_DECAY.maxval, math.random()))
    params:set("env_2_sustain", 0.1 + math.random() * 0.9)
    params:set("env_2_release", util.linlin(0, 1, 0.5, specs.ENV_RELEASE.maxval, math.random()))
    
    params:set("amp", util.linlin(0, 1, 0.5, 0.8, math.random()))
    params:set("amp_mod", math.random())
    
    params:set("ring_mod_fade", util.linlin(0, 1, specs.RING_MOD_FADE.minval, specs.RING_MOD_FADE.maxval, math.random()))
    if(math.random() > 0.8) then
      params:set("ring_mod_mix", math.random())
    else
      params:set("ring_mod_mix", 0)
    end
    
    
  else -- Perc
    
    params:set("freq_mod_lfo", util.linexp(0, 1, 0.0000001, 1, math.pow(math.random(), 2)))
    params:set("freq_mod_env", util.linlin(0, 1, specs.FREQ_MOD_ENV.minval, specs.FREQ_MOD_ENV.maxval, math.pow(math.random(), 4)))
    
    params:set("glide", util.linexp(0, 1, 0.0000001, specs.GLIDE.maxval, math.pow(math.random(), 2)))
    
    params:set("main_osc_level", math.random())
    params:set("sub_osc_level", math.random())
    params:set("sub_osc_detune", util.linlin(0, 1, specs.SUB_OSC_DETUNE.minval, specs.SUB_OSC_DETUNE.maxval, math.random()))
    params:set("noise_level", util.linlin(0, 1, 0.1, 1, math.random()))
    
    params:set("lp_filter_cutoff", util.linexp(0, 1, 100, 6000, math.random()))
    if math.random() > 0.6 then
      params:set("lp_filter_resonance", util.linlin(0, 1, 0.5, 1, math.random()))
    else
      params:set("lp_filter_resonance", math.random())
    end
    params:set("lp_filter_mod_env", util.linlin(0, 1, -0.3, 1, math.random()))
    params:set("lp_filter_mod_lfo", math.random())
    
    params:set("env_1_attack", util.linlin(0, 1, specs.ENV_ATTACK.minval, specs.ENV_ATTACK.maxval, math.random()))
    
    params:set("env_2_attack", specs.ENV_ATTACK.minval)
    params:set("env_2_decay", util.linlin(0, 1, 0.008, 1.8, math.pow(math.random(), 4)))
    params:set("env_2_sustain", 0)
    params:set("env_2_release", params:get("env_2_decay"))
    
    if params:get("env_2_decay") < 0.15 and params:get("env_1_attack") > 1 then
      params:set("env_1_attack", params:get("env_2_decay"))
    end
    
    local amp_max = 1
    if math.random() > 0.7 then amp_max = 11 end
    params:set("amp", util.linlin(0, 1, 0.75, amp_max, math.random()))
    params:set("amp_mod", util.linlin(0, 1, 0, 0.2, math.random()))
    
    params:set("ring_mod_fade", util.linlin(0, 1, specs.RING_MOD_FADE.minval, 2, math.random()))
    if(math.random() > 0.4) then
      params:set("ring_mod_mix", math.random())
    else
      params:set("ring_mod_mix", 0)
    end
    
  end
  
  if params:get("main_osc_level") < 0.6 and params:get("sub_osc_level") < 0.6 and params:get("noise_level") < 0.6 then
    params:set("main_osc_level", util.linlin(0, 1, 0.6, 1, math.random()))
  end
  
  if params:get("lp_filter_cutoff") > 12000 and math.random() > 0.7 then
    params:set("hp_filter_cutoff", util.linexp(0, 1, specs.HP_FILTER_CUTOFF.minval, params:get("lp_filter_cutoff") * 0.05, math.random()))
  else
    params:set("hp_filter_cutoff", specs.HP_FILTER_CUTOFF.minval)
  end
  
  if params:get("lp_filter_cutoff") < 600 and params:get("lp_filter_mod_env") < 0 then
    params:set("lp_filter_mod_env", math.abs(params:get("lp_filter_mod_env")))
  end
  
end

return MollyThePoly
