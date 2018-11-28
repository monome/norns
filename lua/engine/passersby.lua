--- Passersby lib
-- Engine params and functions.
--
-- @module Passersby
-- @release v1.0.0
-- @author Mark Eats

local ControlSpec = require "controlspec"
local MEFormatters = require "mark_eats/formatters"

local Passersby = {}

Passersby.LFO_DESTINATIONS = {"None", "Frequency", "Wave Shape", "Wave Folds", "FM Low", "FM High", "LPG Peak", "LPG Decay", "Reverb Mix"}

local specs = {}

specs.WAVE_SHAPE = ControlSpec.UNIPOLAR
specs.WAVE_FOLDS = ControlSpec.new(0.0, 3.0, "lin", 0, 0)
specs.FM_LOW_AMOUNT = ControlSpec.UNIPOLAR
specs.FM_HIGH_AMOUNT = ControlSpec.UNIPOLAR
specs.LPG_PEAK = ControlSpec.new(100, 10000, "exp", 0, 10000, "Hz")
specs.LPG_DECAY = ControlSpec.new(0.1, 8.0, "exp", 0, 2, "s")
specs.REVERB_MIX = ControlSpec.UNIPOLAR
specs.LFO_FREQ = ControlSpec.new(0.001, 10.0, "exp", 0, 0.5, "Hz")
specs.LFO_AMOUNT = ControlSpec.new(0, 1, "lin", 0, 0.5, "")
specs.DRIFT = ControlSpec.UNIPOLAR

Passersby.specs = specs


function Passersby.add_params()
  
  params:add{type = "control", id = "wave_shape", name = "Wave Shape", controlspec = specs.WAVE_SHAPE, action = engine.waveShape}
  params:add{type = "control", id = "wave_folds", name = "Wave Folds", controlspec = specs.WAVE_FOLDS, action = engine.waveFolds}
  params:add{type = "control", id = "fm_low_amount", name = "FM Low Amount", controlspec = specs.FM_LOW_AMOUNT, action = engine.fm1Amount}
  params:add{type = "control", id = "fm_high_amount", name = "FM High Amount", controlspec = specs.FM_HIGH_AMOUNT, action = engine.fm2Amount}
  params:add{type = "control", id = "lpg_peak", name = "LPG Peak", controlspec = specs.LPG_PEAK, formatter = MEFormatters.format_freq_param, action = engine.lpgPeak}
  params:add{type = "control", id = "lpg_decay", name = "LPG Decay", controlspec = specs.LPG_DECAY, formatter = MEFormatters.format_secs_param, action = engine.lpgDecay}
  params:add{type = "control", id = "reverb_mix", name = "Reverb Mix", controlspec = specs.REVERB_MIX, action = engine.reverbMix}
  params:add{type = "control", id = "lfo_frequency", name = "LFO Frequency", controlspec = specs.LFO_FREQ, formatter = MEFormatters.format_freq_param, action = engine.lfoFreq}
  params:add{type = "control", id = "lfo_amount", name = "LFO Amount", controlspec = specs.LFO_AMOUNT, action = engine.lfoAmount}
  for i = 1, 2 do
    params:add{type = "option", id = "lfo_destination_" .. i, name = "LFO Destination " .. i, options = Passersby.LFO_DESTINATIONS, action = function(value)
      engine.lfoDest(i - 1, value - 1)
    end}
  end
  params:add{type = "control", id = "drift", name = "Drift", controlspec = specs.DRIFT, action = engine.drift}
  
  params:bang()
  
  params:add{type = "trigger", id = "randomize", name = "Randomize", action = Passersby.randomize_params}
  
end

function Passersby.randomize_params()
  params:set("wave_shape", math.random())
  params:set("wave_folds", util.linlin(0, 1, Passersby.specs.WAVE_FOLDS.minval, Passersby.specs.WAVE_FOLDS.maxval, math.pow(math.random(), 2)))
  params:set("fm_low_amount", math.pow(math.random(), 4))
  params:set("fm_high_amount", math.pow(math.random(), 4))
  params:set("lpg_peak", util.linlin(0, 1, Passersby.specs.LPG_PEAK.minval, Passersby.specs.LPG_PEAK.maxval, math.random()))
  params:set("lpg_decay", util.linlin(0, 1, Passersby.specs.LPG_DECAY.minval, Passersby.specs.LPG_DECAY.maxval, math.pow(math.random(), 2)))
  params:set("reverb_mix", math.random())
  params:set("lfo_frequency", util.linlin(0, 1, Passersby.specs.LFO_FREQ.minval, Passersby.specs.LFO_FREQ.maxval, math.random()))
  params:set("lfo_amount", math.random())
  for i = 1, 2 do
    if math.random() > 0.4 then params:set("lfo_destination_" .. i, util.round(util.linlin(0, 1, 1, #Passersby.LFO_DESTINATIONS, math.random())))
    else params:set("lfo_destination_" .. i, 0) end
  end
end

return Passersby
