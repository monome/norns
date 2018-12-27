--- Passersby lib
-- Engine params and functions.
--
-- @module Passersby
-- @release v1.1.0
-- @author Mark Eats

local ControlSpec = require "controlspec"
local Formatters = require "jah/formatters"

local Passersby = {}

local specs = {}

specs.GLIDE = ControlSpec.new(0, 5, "lin", 0, 0, "s")
specs.WAVE_SHAPE = ControlSpec.UNIPOLAR
specs.WAVE_FOLDS = ControlSpec.new(0.0, 3.0, "lin", 0, 0)
specs.FM_LOW_RATIO = ControlSpec.new(0.1, 1, "lin", 0, 0.66)
specs.FM_HIGH_RATIO = ControlSpec.new(1, 10, "lin", 0, 3.3)
specs.FM_LOW_AMOUNT = ControlSpec.UNIPOLAR
specs.FM_HIGH_AMOUNT = ControlSpec.UNIPOLAR
specs.ATTACK = ControlSpec.new(0.003, 8, "exp", 0, 0.04, "s")
specs.PEAK = ControlSpec.new(100, 10000, "exp", 0, 10000, "Hz")
specs.DECAY = ControlSpec.new(0.01, 8, "exp", 0, 1, "s")
specs.AMP = ControlSpec.new(0, 11, "lin", 0, 1, "")
specs.REVERB_MIX = ControlSpec.UNIPOLAR
specs.LFO_FREQ = ControlSpec.new(0.001, 10.0, "exp", 0, 0.5, "Hz")
specs.LFO_AMOUNT = ControlSpec.UNIPOLAR
specs.DRIFT = ControlSpec.UNIPOLAR

Passersby.specs = specs


local function format_wave_shape(param)
  local value = param:get()
  local wave_names = {}
  
  if value < 0.28 then table.insert(wave_names, "Sine") end
  if value > 0.05 and value < 0.64 then table.insert(wave_names, "Tri") end
  if value > 0.38 and value < 0.95 then table.insert(wave_names, "Sqr") end
  if value > 0.71 then table.insert(wave_names, "Saw") end
  
  local return_string = ""
  for i = 1, #wave_names do
    if i > 1 then return_string = return_string .. "/" end
    return_string = return_string .. wave_names[i]
  end
  return return_string .. " " .. util.round(value, 0.01)
end

local function format_attack(param)
  local return_string
  if params:get("env_type") == 1 then
    return_string = "N/A"
  else
    return_string = Formatters.format_secs(param)
  end
  return return_string
end

function Passersby.add_params()
  
  params:add{type = "control", id = "glide", name = "Glide", controlspec = specs.GLIDE, action = engine.glide}
  params:add{type = "control", id = "wave_shape", name = "Wave Shape", controlspec = specs.WAVE_SHAPE, formatter = format_wave_shape, action = engine.waveShape}
  params:add{type = "control", id = "wave_folds", name = "Wave Folds", controlspec = specs.WAVE_FOLDS, action = engine.waveFolds}
  params:add{type = "control", id = "fm_low_ratio", name = "FM Low Ratio", controlspec = specs.FM_LOW_RATIO, action = engine.fm1Ratio}
  params:add{type = "control", id = "fm_high_ratio", name = "FM High Ratio", controlspec = specs.FM_HIGH_RATIO, action = engine.fm2Ratio}
  params:add{type = "control", id = "fm_low_amount", name = "FM Low Amount", controlspec = specs.FM_LOW_AMOUNT, action = engine.fm1Amount}
  params:add{type = "control", id = "fm_high_amount", name = "FM High Amount", controlspec = specs.FM_HIGH_AMOUNT, action = engine.fm2Amount}
  params:add{type = "option", id = "env_type", name = "Envelope Type", options = {"LPG", "Sustain"}, action = function(value)
    engine.envType(value - 1)
  end}
  params:add{type = "control", id = "attack", name = "Attack", controlspec = specs.ATTACK, formatter = format_attack, action = engine.attack}
  params:add{type = "control", id = "peak", name = "Peak", controlspec = specs.PEAK, formatter = Formatters.format_freq, action = engine.peak}
  params:add{type = "control", id = "decay", name = "Decay", controlspec = specs.DECAY, formatter = Formatters.format_secs, action = engine.decay}
  params:add{type = "control", id = "amp", name = "Amp", controlspec = specs.AMP, action = engine.amp}
  params:add{type = "control", id = "reverb_mix", name = "Reverb Mix", controlspec = specs.REVERB_MIX, action = engine.reverbMix}
  params:add{type = "option", id = "lfo_shape", name = "LFO Shape", options = {"Triangle", "Ramp", "Square", "Random"}, action = function(value)
    engine.lfoShape(value - 1)
  end}
  params:add{type = "control", id = "lfo_freq", name = "LFO Frequency", controlspec = specs.LFO_FREQ, formatter = Formatters.format_freq, action = engine.lfoFreq}
  params:add{type = "control", id = "lfo_to_freq_amount", name = "LFO > Frequency", controlspec = specs.LFO_AMOUNT, action = engine.lfoToFreqAmount}
  params:add{type = "control", id = "lfo_to_wave_shape_amount", name = "LFO > Wave Shape", controlspec = specs.LFO_AMOUNT, action = engine.lfoToWaveShapeAmount}
  params:add{type = "control", id = "lfo_to_wave_folds_amount", name = "LFO > Wave Folds", controlspec = specs.LFO_AMOUNT, action = engine.lfoToWaveFoldsAmount}
  params:add{type = "control", id = "lfo_to_fm_low_amount", name = "LFO > FM Low", controlspec = specs.LFO_AMOUNT, action = engine.lfoToFm1Amount}
  params:add{type = "control", id = "lfo_to_fm_high_amount", name = "LFO > FM High", controlspec = specs.LFO_AMOUNT, action = engine.lfoToFm2Amount}
  params:add{type = "control", id = "lfo_to_attack_amount", name = "LFO > Attack", controlspec = specs.LFO_AMOUNT, action = engine.lfoToAttackAmount}
  params:add{type = "control", id = "lfo_to_peak_amount", name = "LFO > Peak", controlspec = specs.LFO_AMOUNT, action = engine.lfoToPeakAmount}
  params:add{type = "control", id = "lfo_to_decay_amount", name = "LFO > Decay", controlspec = specs.LFO_AMOUNT, action = engine.lfoToDecayAmount}
  params:add{type = "control", id = "lfo_to_reverb_mix_amount", name = "LFO > Reverb Mix", controlspec = specs.LFO_AMOUNT, action = engine.lfoToReverbMixAmount}
  params:add{type = "control", id = "drift", name = "Drift", controlspec = specs.DRIFT, action = engine.drift}
  
  params:bang()
  
  params:add{type = "trigger", id = "randomize", name = "Randomize", action = Passersby.randomize_params}
  
end

function Passersby.randomize_params()
  if math.random() > 0.8 then params:set("glide", util.linlin(0, 1, Passersby.specs.GLIDE.minval, Passersby.specs.GLIDE.maxval,  math.pow(math.random(), 2))) else params:set("glide", 0) end
  params:set("wave_shape", math.random())
  params:set("wave_folds", util.linlin(0, 1, Passersby.specs.WAVE_FOLDS.minval, Passersby.specs.WAVE_FOLDS.maxval, math.pow(math.random(), 2)))
  if math.random() > 0.55 then params:set("fm_low_amount", math.pow(math.random(), 4)) else params:set("fm_low_amount", 0) end
  if math.random() > 0.55 then params:set("fm_high_amount", math.pow(math.random(), 4)) else params:set("fm_high_amount", 0) end
  params:set("env_type", math.random(1, 2))
  params:set("attack", util.linlin(0, 1, Passersby.specs.ATTACK.minval, Passersby.specs.ATTACK.maxval, math.pow(math.random(), 4)))
  params:set("peak", util.linlin(0, 1, Passersby.specs.PEAK.minval, Passersby.specs.PEAK.maxval, math.random()))
  params:set("decay", util.linlin(0, 1, Passersby.specs.DECAY.minval, Passersby.specs.DECAY.maxval, math.pow(math.random(), 2)))
  if math.random() > 0.5 then params:set("amp", util.linlin(0, 1, 0.7, Passersby.specs.AMP.maxval, math.pow(math.random(), 4))) else params:set("amp", 1) end
  params:set("reverb_mix", math.random())
  params:set("lfo_shape", math.random(1, 4))
  params:set("lfo_freq", util.linlin(0, 1, Passersby.specs.LFO_FREQ.minval, Passersby.specs.LFO_FREQ.maxval, math.random()))
  local LFO_RAND_THRESHOLD = 0.65
  if math.random() > 0.75 then params:set("lfo_to_freq_amount", math.pow(math.random(), 2)) else params:set("lfo_to_freq_amount", 0) end
  if math.random() > LFO_RAND_THRESHOLD then params:set("lfo_to_wave_shape_amount", math.random()) else params:set("lfo_to_wave_shape_amount", 0) end
  if math.random() > LFO_RAND_THRESHOLD then params:set("lfo_to_wave_folds_amount", math.random()) else params:set("lfo_to_wave_folds_amount", 0) end
  if math.random() > 0.75 then params:set("lfo_to_fm_low_amount", math.random()) else params:set("lfo_to_fm_low_amount", 0) end
  if math.random() > 0.75 then params:set("lfo_to_fm_high_amount", math.random()) else params:set("lfo_to_fm_high_amount", 0) end
  if math.random() > LFO_RAND_THRESHOLD then params:set("lfo_to_attack_amount", math.random()) else params:set("lfo_to_attack_amount", 0) end
  if math.random() > LFO_RAND_THRESHOLD then params:set("lfo_to_peak_amount", math.random()) else params:set("lfo_to_peak_amount", 0) end
  if math.random() > LFO_RAND_THRESHOLD then params:set("lfo_to_decay_amount", math.random()) else params:set("lfo_to_decay_amount", 0) end
  if math.random() > LFO_RAND_THRESHOLD then params:set("lfo_to_reverb_mix_amount", math.random()) else params:set("lfo_to_reverb_mix_amount", 0) end
end

return Passersby
