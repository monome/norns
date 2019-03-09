local R = {}

local ControlSpec = require 'controlspec'

local eng = {} -- NOTE: functions rely on engine global
local util = {}
local specs = {}

specs['44Matrix'] = {
	FadeTime = ControlSpec.new(0, 1000, "linear", 0, 5, "ms"),
	Gate_1_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_1_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_1_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_1_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_4 = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['88Matrix'] = {
	FadeTime = ControlSpec.new(0, 1000, "linear", 0, 5, "ms"),
	Gate_1_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_1_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_1_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_1_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_1_5 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_1_6 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_1_7 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_1_8 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_5 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_6 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_7 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_2_8 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_5 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_6 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_7 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_3_8 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_5 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_6 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_7 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_4_8 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_5_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_5_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_5_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_5_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_5_5 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_5_6 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_5_7 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_5_8 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_6_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_6_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_6_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_6_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_6_5 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_6_6 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_6_7 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_6_8 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_7_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_7_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_7_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_7_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_7_5 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_7_6 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_7_7 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_7_8 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_8_1 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_8_2 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_8_3 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_8_4 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_8_5 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_8_6 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_8_7 = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Gate_8_8 = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['ADSREnv'] = {
	Attack = ControlSpec.new(0.1, 2000, "exp", 0, 5, "ms"),
	Decay = ControlSpec.new(0.1, 8000, "exp", 0, 200, "ms"),
	Sustain = ControlSpec.new(0, 1, "linear", 0, 0.5, ""),
	Release = ControlSpec.new(0.1, 8000, "exp", 0, 200, "ms"),
	Gate = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['Amp'] = {
	Level = ControlSpec.UNIPOLAR
}

specs['Amp2'] = {
	Gain = ControlSpec.UNIPOLAR,
	GainModulation = ControlSpec.BIPOLAR,
	In1 = ControlSpec.UNIPOLAR,
	In2 = ControlSpec.UNIPOLAR,
	Out = ControlSpec.UNIPOLAR,
	Mode = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['DbMixer'] = {
	In1 = ControlSpec.DB,
	In2 = ControlSpec.DB,
	In3 = ControlSpec.DB,
	In4 = ControlSpec.DB,
	Out = ControlSpec.DB
}

specs['Delay'] = {
	DelayTime = ControlSpec.new(0.1, 5000, "exp", 0, 300, "ms"),
	DelayTimeModulation = ControlSpec.BIPOLAR
}

specs['FMVoice'] = {
	Freq = ControlSpec.FREQ,
	Timbre = ControlSpec.new(0, 5, "linear", 0, 1, ""),
	Osc1Gain = ControlSpec.AMP,
	Osc1Partial = ControlSpec.new(0.5, 12, "linear", 0.5, 1, ""),
	Osc1Fixed = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Osc1Fixedfreq = ControlSpec.WIDEFREQ,
	Osc1Index = ControlSpec.new(0, 24, "linear", 0, 3, ""),
	Osc1Outlevel = ControlSpec.AMP,
	Osc1_To_Osc1Freq = ControlSpec.AMP,
	Osc1_To_Osc2Freq = ControlSpec.AMP,
	Osc1_To_Osc3Freq = ControlSpec.AMP,
	Osc2Gain = ControlSpec.AMP,
	Osc2Partial = ControlSpec.new(0.5, 12, "linear", 0.5, 1, ""),
	Osc2Fixed = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Osc2Fixedfreq = ControlSpec.WIDEFREQ,
	Osc2Index = ControlSpec.new(0, 24, "linear", 0, 3, ""),
	Osc2Outlevel = ControlSpec.AMP,
	Osc2_To_Osc1Freq = ControlSpec.AMP,
	Osc2_To_Osc2Freq = ControlSpec.AMP,
	Osc2_To_Osc3Freq = ControlSpec.AMP,
	Osc3Gain = ControlSpec.AMP,
	Osc3Partial = ControlSpec.new(0.5, 12, "linear", 0.5, 1, ""),
	Osc3Fixed = ControlSpec.new(0, 1, "linear", 1, 0, ""),
	Osc3Fixedfreq = ControlSpec.WIDEFREQ,
	Osc3Index = ControlSpec.new(0, 24, "linear", 0, 3, ""),
	Osc3Outlevel = ControlSpec.AMP,
	Osc3_To_Osc3Freq = ControlSpec.AMP,
	Osc3_To_Osc2Freq = ControlSpec.AMP,
	Osc3_To_Osc1Freq = ControlSpec.AMP,
	Mod_To_Osc1Gain = ControlSpec.BIPOLAR,
	Mod_To_Osc2Gain = ControlSpec.BIPOLAR,
	Mod_To_Osc3Gain = ControlSpec.BIPOLAR,
	Mod_To_Osc1Freq = ControlSpec.BIPOLAR,
	Mod_To_Osc2Freq = ControlSpec.BIPOLAR,
	Mod_To_Osc3Freq = ControlSpec.BIPOLAR
}

specs['FShift'] = {
	Frequency = ControlSpec.new(-2000, 2000, "linear", 0, 0, "Hz"),
	FM = ControlSpec.BIPOLAR
}

specs['FreqGate'] = {
	Frequency = ControlSpec.FREQ,
	Gate = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['LPFilter'] = {
	AudioLevel = ControlSpec.AMP,
	Frequency = ControlSpec.WIDEFREQ,
	Resonance = ControlSpec.UNIPOLAR,
	FM = ControlSpec.BIPOLAR,
	ResonanceModulation = ControlSpec.BIPOLAR
}

specs['LPLadder'] = {
	Frequency = ControlSpec.WIDEFREQ,
	Resonance = ControlSpec.UNIPOLAR,
	FM = ControlSpec.BIPOLAR,
	ResonanceModulation = ControlSpec.BIPOLAR
}

specs['LinMixer'] = {
	In1 = ControlSpec.UNIPOLAR,
	In2 = ControlSpec.UNIPOLAR,
	In3 = ControlSpec.UNIPOLAR,
	In4 = ControlSpec.UNIPOLAR,
	Out = ControlSpec.UNIPOLAR
}

specs['MGain'] = {
	Gain = ControlSpec.new(-math.huge, 12, "db", 0, 0, " dB"),
	Mute = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['MMFilter'] = {
	AudioLevel = ControlSpec.AMP,
	Frequency = ControlSpec.WIDEFREQ,
	Resonance = ControlSpec.UNIPOLAR,
	FM = ControlSpec.BIPOLAR,
	ResonanceModulation = ControlSpec.BIPOLAR
}

specs['MultiLFO'] = {
	Frequency = ControlSpec.new(0.01, 50, "exp", 0, 1, "Hz"),
	Reset = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['MultiOsc'] = {
	Range = ControlSpec.new(-2, 2, "linear", 1, 0, ""),
	Tune = ControlSpec.new(-600, 600, "linear", 0, 0, "cents"),
	FM = ControlSpec.UNIPOLAR,
	PulseWidth = ControlSpec.new(0, 1, "linear", 0, 0.5, ""),
	PWM = ControlSpec.UNIPOLAR
}

specs['Noise'] = {
}

specs['OGain'] = {
	Gain = ControlSpec.new(-math.huge, 12, "db", 0, 0, " dB"),
	Mute = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['PShift'] = {
	PitchRatio = ControlSpec.new(0, 4, "linear", 0, 1, ""),
	PitchDispersion = ControlSpec.new(0, 4, "linear", 0, 0, ""),
	TimeDispersion = ControlSpec.new(0, 1, "linear", 0, 0, ""),
	PitchRatioModulation = ControlSpec.BIPOLAR,
	PitchDispersionModulation = ControlSpec.BIPOLAR,
	TimeDispersionModulation = ControlSpec.BIPOLAR
}

specs['PulseOsc'] = {
	Range = ControlSpec.new(-2, 2, "linear", 1, 0, ""),
	Tune = ControlSpec.new(-600, 600, "linear", 0, 0, "cents"),
	FM = ControlSpec.UNIPOLAR,
	PulseWidth = ControlSpec.new(0, 1, "linear", 0, 0.5, ""),
	PWM = ControlSpec.new(0, 1, "linear", 0, 0.4, "")
}

specs['QGain'] = {
	Gain = ControlSpec.new(-math.huge, 12, "db", 0, 0, " dB"),
	Mute = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['RingMod'] = {
}

specs['SGain'] = {
	Gain = ControlSpec.new(-math.huge, 12, "db", 0, 0, " dB"),
	Mute = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['SampHold'] = {
}

specs['SawOsc'] = {
	Range = ControlSpec.new(-2, 2, "linear", 1, 0, ""),
	Tune = ControlSpec.new(-600, 600, "linear", 0, 0, "cents"),
	FM = ControlSpec.UNIPOLAR
}

specs['SineLFO'] = {
	Frequency = ControlSpec.new(0.01, 50, "exp", 0, 1, "Hz"),
	Reset = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['SineOsc'] = {
	Range = ControlSpec.new(-2, 2, "linear", 1, 0, ""),
	Tune = ControlSpec.new(-600, 600, "linear", 0, 0, "cents"),
	FM = ControlSpec.UNIPOLAR
}

specs['SoundIn'] = {
}

specs['SoundOut'] = {
}

specs['TestGen'] = {
	Frequency = ControlSpec.WIDEFREQ,
	Amplitude = ControlSpec.DB,
	Wave = ControlSpec.new(0, 1, "linear", 1, 0, "")
}

specs['TriOsc'] = {
	Range = ControlSpec.new(-2, 2, "linear", 1, 0, ""),
	Tune = ControlSpec.new(-600, 600, "linear", 0, 0, "cents"),
	FM = ControlSpec.UNIPOLAR
}

specs['XFader'] = {
	Fade = ControlSpec.BIPOLAR,
	TrimA = ControlSpec.new(-math.huge, 12, "db", 0, -math.huge, " dB"),
	TrimB = ControlSpec.new(-math.huge, 12, "db", 0, -math.huge, " dB"),
	Master = ControlSpec.new(-math.huge, 12, "db", 0, -math.huge, " dB")
}

-- utility function to create module, will validate kind lua side
function eng.new(name, kind)
  if specs[kind] then
    engine.new(name..voicenum, kind)
  else
    error(kind.." - not a valid module type")
  end
end

-- utility function to create multiple modules suffixed 1..polyphony
function eng.poly_new(name, kind, polyphony)
  if specs[kind] then
    for voicenum=1, polyphony do
      engine.new(name..voicenum, kind)
    end
  else
    error(kind.." not a valid module type")
  end
end

-- utility function to connect modules suffixed with 1..polyphony
function eng.poly_connect(output, input, polyphony)
  local sourcemodule, outputref = util.split_ref(output)
  local destmodule, inputref = util.split_ref(input)
  for voicenum=1, polyphony do
    engine.connect(sourcemodule..voicenum.."/"..outputref, destmodule..voicenum.."/"..inputref)
  end
end

-- utility function to set param of multiple modules suffixed 1..polyphony
function eng.poly_set(ref, value, polyphony)
  local module, param = util.split_ref(ref)
  for voicenum=1, polyphony do
    engine.set(util.param_voice_ref(module, param, voicenum), value)
  end
end

-- utility function to expand a moduleparam ref to #polyphony ones suffixed with 1..polyphony
function util.poly_expand(moduleparam, polyphony)
  local module, param = util.split_ref(moduleparam)
  local expanded = ""

  for voicenum=1, polyphony do
    expanded = expanded .. util.param_voice_ref(module, param, voicenum)
    if voicenum ~= polyphony then
      expanded = expanded .. " "
    end
  end

  return expanded
end

function util.param_voice_ref(module, param, voicenum)
  return module .. voicenum .. "." .. param
end

function util.split_ref(ref)
  local words = {}
  for word in ref:gmatch("[a-zA-Z0-9]+") do table.insert(words, word) end
  return words[1], words[2]
end

R.specs = specs
R.engine = eng
R.util = util

return R
