-- FM 7 lib
-- Engine params and functions
--
-- @module FM7
-- @release v0.0.1
-- @author Lee Azzarello

local ControlSpec = require "controlspec"

local FM7 = {}

local specs = {}
local options = {}

specs.OPAMP_A1= ControlSpec.new(0.01,10,"lin",0,0.05,"ms")
specs.OPAMP_D1= ControlSpec.new(0,2,"lin",0,0.1,"ms")
specs.OPAMP_S1= ControlSpec.new(0,1,"lin",0,1,"db")
specs.OPAMP_R1= ControlSpec.new(0.01,10,"lin",0,1,"ms")

specs.OPAMP_A2= ControlSpec.new(0.01,10,"lin",0,0.05,"ms")
specs.OPAMP_D2= ControlSpec.new(0,2,"lin",0,0.1,"ms")
specs.OPAMP_S2= ControlSpec.new(0,1,"lin",0,1,"db")
specs.OPAMP_R2= ControlSpec.new(0.01,10,"lin",0,1,"ms")

specs.OPAMP_A3= ControlSpec.new(0.01,10,"lin",0,0.05,"ms")
specs.OPAMP_D3= ControlSpec.new(0,2,"lin",0,0.1,"ms")
specs.OPAMP_S3= ControlSpec.new(0,1,"lin",0,1,"db")
specs.OPAMP_R3= ControlSpec.new(0.01,10,"lin",0,1,"ms")

specs.OPAMP_A4= ControlSpec.new(0.01,10,"lin",0,0.05,"ms")
specs.OPAMP_D4= ControlSpec.new(0,2,"lin",0,0.1,"ms")
specs.OPAMP_S4= ControlSpec.new(0,1,"lin",0,1,"db")
specs.OPAMP_R4= ControlSpec.new(0.01,10,"lin",0,1,"ms")

specs.OPAMP_A5= ControlSpec.new(0.01,10,"lin",0,0.05,"ms")
specs.OPAMP_D5= ControlSpec.new(0,2,"lin",0,0.1,"ms")
specs.OPAMP_S5= ControlSpec.new(0,1,"lin",0,1,"db")
specs.OPAMP_R5= ControlSpec.new(0.01,10,"lin",0,1,"ms")

specs.OPAMP_A6= ControlSpec.new(0.01,10,"lin",0,0.05,"ms")
specs.OPAMP_D6= ControlSpec.new(0,2,"lin",0,0.1,"ms")
specs.OPAMP_S6= ControlSpec.new(0,1,"lin",0,1,"db")
specs.OPAMP_R6= ControlSpec.new(0.01,10,"lin",0,1,"ms")

specs.HZ1 = ControlSpec.new(0,5, "lin",0,1,"")
specs.HZ2 = ControlSpec.new(0,5, "lin",0,1,"")
specs.HZ3 = ControlSpec.new(0,5, "lin",0,1,"")
specs.HZ4 = ControlSpec.new(0,5, "lin",0,1,"")
specs.HZ5 = ControlSpec.new(0,5, "lin",0,1,"")
specs.HZ6 = ControlSpec.new(0,5, "lin",0,1,"")
specs.PHASE1 = ControlSpec.new(0,2*math.pi, "lin",0,0,"rad")
specs.PHASE2 = ControlSpec.new(0,2*math.pi, "lin",0,0,"rad")
specs.PHASE3 = ControlSpec.new(0,2*math.pi, "lin",0,0,"rad")
specs.PHASE4 = ControlSpec.new(0,2*math.pi, "lin",0,0,"rad")
specs.PHASE5 = ControlSpec.new(0,2*math.pi, "lin",0,0,"rad")
specs.PHASE6 = ControlSpec.new(0,2*math.pi, "lin",0,0,"rad")
specs.AMP1 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.AMP2 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.AMP3 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.AMP4 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.AMP5 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.AMP6 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.HZ1_TO_HZ1 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ1_TO_HZ2 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ1_TO_HZ3 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ1_TO_HZ4 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ1_TO_HZ5 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ1_TO_HZ6 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ2_TO_HZ1 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ2_TO_HZ2 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ2_TO_HZ3 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ2_TO_HZ4 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ2_TO_HZ5 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ2_TO_HZ6 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ3_TO_HZ1 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ3_TO_HZ2 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ3_TO_HZ3 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ3_TO_HZ4 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ3_TO_HZ5 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ3_TO_HZ6 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ4_TO_HZ1 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ4_TO_HZ2 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ4_TO_HZ3 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ4_TO_HZ4 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ4_TO_HZ5 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ4_TO_HZ6 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ5_TO_HZ1 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ5_TO_HZ2 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ5_TO_HZ3 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ5_TO_HZ4 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ5_TO_HZ5 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ5_TO_HZ6 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ6_TO_HZ1 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ6_TO_HZ2 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ6_TO_HZ3 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ6_TO_HZ4 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ6_TO_HZ5 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.HZ6_TO_HZ6 = ControlSpec.new(0,2*math.pi,"lin",0,0,"rad")
specs.CARRIER1 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.CARRIER2 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.CARRIER3 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.CARRIER4 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.CARRIER5 = ControlSpec.new(0,1, "lin",0,1,"db")
specs.CARRIER6 = ControlSpec.new(0,1, "lin",0,1,"db")

FM7.specs = specs

function FM7.add_params()
  params:add{type = "control", id = "hz1",name = "Osc 1 Frequency Multiplier", controlspec = specs.HZ1, action = engine.hz1}
  params:add{type = "control", id = "hz2",name = "Osc 2 Frequency Multiplier", controlspec = specs.HZ2, action = engine.hz2}
  params:add{type = "control", id = "hz3",name = "Osc 3 Frequency Multiplier", controlspec = specs.HZ3, action = engine.hz3}
  params:add{type = "control", id = "hz4",name = "Osc 4 Frequency Multiplier", controlspec = specs.HZ4, action = engine.hz4}
  params:add{type = "control", id = "hz5",name = "Osc 5 Frequency Multiplier", controlspec = specs.HZ5, action = engine.hz5}
  params:add{type = "control", id = "hz6",name = "Osc 6 Frequency Multiplier", controlspec = specs.HZ6, action = engine.hz6}
  params:add_separator()
  params:add{type = "control", id = "phase1",name = "Osc 1 Phase", controlspec = specs.PHASE1, action = engine.phase1}
  params:add{type = "control", id = "phase2",name = "Osc 2 Phase", controlspec = specs.PHASE2, action = engine.phase2}
  params:add{type = "control", id = "phase3",name = "Osc 3 Phase", controlspec = specs.PHASE3, action = engine.phase3}
  params:add{type = "control", id = "phase4",name = "Osc 4 Phase", controlspec = specs.PHASE4, action = engine.phase4}
  params:add{type = "control", id = "phase5",name = "Osc 5 Phase", controlspec = specs.PHASE5, action = engine.phase5}
  params:add{type = "control", id = "phase6",name = "Osc 6 Phase", controlspec = specs.PHASE6, action = engine.phase6}
  params:add_separator()
  params:add{type = "control", id = "amp1",name = "Osc 1 Amplitude", controlspec = specs.AMP1, action = engine.amp1}
  params:add{type = "control", id = "amp2",name = "Osc 2 Amplitude", controlspec = specs.AMP2, action = engine.amp2}
  params:add{type = "control", id = "amp3",name = "Osc 3 Amplitude", controlspec = specs.AMP3, action = engine.amp3}
  params:add{type = "control", id = "amp4",name = "Osc 4 Amplitude", controlspec = specs.AMP4, action = engine.amp4}
  params:add{type = "control", id = "amp5",name = "Osc 5 Amplitude", controlspec = specs.AMP5, action = engine.amp5}
  params:add{type = "control", id = "amp6",name = "Osc 6 Amplitude", controlspec = specs.AMP6, action = engine.amp6}
  params:add_separator()
  params:add{type = "control", id = "hz1_to_hz1",name = "Osc1 Phase Mod Osc1", controlspec = specs.HZ1_TO_HZ1, action = engine.hz1_to_hz1}
  params:add{type = "control", id = "hz1_to_hz2",name = "Osc1 Phase Mod Osc2", controlspec = specs.HZ1_TO_HZ2, action = engine.hz1_to_hz2}
  params:add{type = "control", id = "hz1_to_hz3",name = "Osc1 Phase Mod Osc3", controlspec = specs.HZ1_TO_HZ3, action = engine.hz1_to_hz3}
  params:add{type = "control", id = "hz1_to_hz4",name = "Osc1 Phase Mod Osc4", controlspec = specs.HZ1_TO_HZ4, action = engine.hz1_to_hz4}
  params:add{type = "control", id = "hz1_to_hz5",name = "Osc1 Phase Mod Osc5", controlspec = specs.HZ1_TO_HZ5, action = engine.hz1_to_hz5}
  params:add{type = "control", id = "hz1_to_hz6",name = "Osc1 Phase Mod Osc6", controlspec = specs.HZ1_TO_HZ6, action = engine.hz1_to_hz6}
  params:add{type = "control", id = "hz2_to_hz1",name = "Osc2 Phase Mod Osc1", controlspec = specs.HZ2_TO_HZ1, action = engine.hz2_to_hz1}
  params:add{type = "control", id = "hz2_to_hz2",name = "Osc2 Phase Mod Osc2", controlspec = specs.HZ2_TO_HZ2, action = engine.hz2_to_hz2}
  params:add{type = "control", id = "hz2_to_hz3",name = "Osc2 Phase Mod Osc3", controlspec = specs.HZ2_TO_HZ3, action = engine.hz2_to_hz3}
  params:add{type = "control", id = "hz2_to_hz4",name = "Osc2 Phase Mod Osc4", controlspec = specs.HZ2_TO_HZ4, action = engine.hz2_to_hz4}
  params:add{type = "control", id = "hz2_to_hz5",name = "Osc2 Phase Mod Osc5", controlspec = specs.HZ2_TO_HZ5, action = engine.hz2_to_hz5}
  params:add{type = "control", id = "hz2_to_hz6",name = "Osc2 Phase Mod Osc6", controlspec = specs.HZ2_TO_HZ6, action = engine.hz2_to_hz6}
  params:add{type = "control", id = "hz3_to_hz1",name = "Osc3 Phase Mod Osc1", controlspec = specs.HZ3_TO_HZ1, action = engine.hz3_to_hz1}
  params:add{type = "control", id = "hz3_to_hz2",name = "Osc3 Phase Mod Osc2", controlspec = specs.HZ3_TO_HZ2, action = engine.hz3_to_hz2}
  params:add{type = "control", id = "hz3_to_hz3",name = "Osc3 Phase Mod Osc3", controlspec = specs.HZ3_TO_HZ3, action = engine.hz3_to_hz3}
  params:add{type = "control", id = "hz3_to_hz4",name = "Osc3 Phase Mod Osc4", controlspec = specs.HZ3_TO_HZ4, action = engine.hz3_to_hz4}
  params:add{type = "control", id = "hz3_to_hz5",name = "Osc3 Phase Mod Osc5", controlspec = specs.HZ3_TO_HZ5, action = engine.hz3_to_hz5}
  params:add{type = "control", id = "hz3_to_hz6",name = "Osc3 Phase Mod Osc6", controlspec = specs.HZ3_TO_HZ6, action = engine.hz3_to_hz6}
  params:add{type = "control", id = "hz4_to_hz1",name = "Osc4 Phase Mod Osc1", controlspec = specs.HZ4_TO_HZ1, action = engine.hz4_to_hz1}
  params:add{type = "control", id = "hz4_to_hz2",name = "Osc4 Phase Mod Osc2", controlspec = specs.HZ4_TO_HZ2, action = engine.hz4_to_hz2}
  params:add{type = "control", id = "hz4_to_hz3",name = "Osc4 Phase Mod Osc3", controlspec = specs.HZ4_TO_HZ3, action = engine.hz4_to_hz3}
  params:add{type = "control", id = "hz4_to_hz4",name = "Osc4 Phase Mod Osc4", controlspec = specs.HZ4_TO_HZ4, action = engine.hz4_to_hz4}
  params:add{type = "control", id = "hz4_to_hz5",name = "Osc4 Phase Mod Osc5", controlspec = specs.HZ4_TO_HZ5, action = engine.hz4_to_hz5}
  params:add{type = "control", id = "hz4_to_hz6",name = "Osc4 Phase Mod Osc6", controlspec = specs.HZ4_TO_HZ6, action = engine.hz4_to_hz6}
  params:add{type = "control", id = "hz5_to_hz1",name = "Osc5 Phase Mod Osc1", controlspec = specs.HZ5_TO_HZ1, action = engine.hz5_to_hz1}
  params:add{type = "control", id = "hz5_to_hz2",name = "Osc5 Phase Mod Osc2", controlspec = specs.HZ5_TO_HZ2, action = engine.hz5_to_hz2}
  params:add{type = "control", id = "hz5_to_hz3",name = "Osc5 Phase Mod Osc3", controlspec = specs.HZ5_TO_HZ3, action = engine.hz5_to_hz3}
  params:add{type = "control", id = "hz5_to_hz4",name = "Osc5 Phase Mod Osc4", controlspec = specs.HZ5_TO_HZ4, action = engine.hz5_to_hz4}
  params:add{type = "control", id = "hz5_to_hz5",name = "Osc5 Phase Mod Osc5", controlspec = specs.HZ5_TO_HZ5, action = engine.hz5_to_hz5}
  params:add{type = "control", id = "hz5_to_hz6",name = "Osc5 Phase Mod Osc6", controlspec = specs.HZ5_TO_HZ6, action = engine.hz5_to_hz6}
  params:add{type = "control", id = "hz6_to_hz1",name = "Osc6 Phase Mod Osc1", controlspec = specs.HZ6_TO_HZ1, action = engine.hz6_to_hz1}
  params:add{type = "control", id = "hz6_to_hz2",name = "Osc6 Phase Mod Osc2", controlspec = specs.HZ6_TO_HZ2, action = engine.hz6_to_hz2}
  params:add{type = "control", id = "hz6_to_hz3",name = "Osc6 Phase Mod Osc3", controlspec = specs.HZ6_TO_HZ3, action = engine.hz6_to_hz3}
  params:add{type = "control", id = "hz6_to_hz4",name = "Osc6 Phase Mod Osc4", controlspec = specs.HZ6_TO_HZ4, action = engine.hz6_to_hz4}
  params:add{type = "control", id = "hz6_to_hz5",name = "Osc6 Phase Mod Osc5", controlspec = specs.HZ6_TO_HZ5, action = engine.hz6_to_hz5}
  params:add{type = "control", id = "hz6_to_hz6",name = "Osc6 Phase Mod Osc6", controlspec = specs.HZ6_TO_HZ6, action = engine.hz6_to_hz6}
  params:add_separator()
  params:add{type = "control", id = "carrier1",name = "Carrier 1 Amplitude", controlspec = specs.CARRIER1, action = engine.carrier1}
  params:add{type = "control", id = "carrier2",name = "Carrier 2 Amplitude", controlspec = specs.CARRIER2, action = engine.carrier2}
  params:add{type = "control", id = "carrier3",name = "Carrier 3 Amplitude", controlspec = specs.CARRIER3, action = engine.carrier3}
  params:add{type = "control", id = "carrier4",name = "Carrier 4 Amplitude", controlspec = specs.CARRIER4, action = engine.carrier4}
  params:add{type = "control", id = "carrier5",name = "Carrier 5 Amplitude", controlspec = specs.CARRIER5, action = engine.carrier5}
  params:add{type = "control", id = "carrier6",name = "Carrier 6 Amplitude", controlspec = specs.CARRIER6, action = engine.carrier6}
  params:add_separator()
  params:add{type = "control", id = "opAmpA1",name = "Osc1 Amp Env Attack", controlspec = specs.OPAMP_A1, action = engine.opAmpA1}
  params:add{type = "control", id = "opAmpD1",name = "Osc1 Amp Env Decay", controlspec = specs.OPAMP_D1, action = engine.opAmpD1}
  params:add{type = "control", id = "opAmpS1",name = "Osc1 Amp Env Sustain", controlspec = specs.OPAMP_S1, action = engine.opAmpS1}
  params:add{type = "control", id = "opAmpR1",name = "Osc1 Amp Env Release", controlspec = specs.OPAMP_R1, action = engine.opAmpR1}
  params:add{type = "control", id = "opAmpA2",name = "Osc2 Amp Env Attack", controlspec = specs.OPAMP_A2, action = engine.opAmpA2}
  params:add{type = "control", id = "opAmpD2",name = "Osc2 Amp Env Decay", controlspec = specs.OPAMP_D2, action = engine.opAmpD2}
  params:add{type = "control", id = "opAmpS2",name = "Osc2 Amp Env Sustain", controlspec = specs.OPAMP_S2, action = engine.opAmpS2}
  params:add{type = "control", id = "opAmpR2",name = "Osc2 Amp Env Release", controlspec = specs.OPAMP_R2, action = engine.opAmpR2}
  params:add{type = "control", id = "opAmpA3",name = "Osc3 Amp Env Attack", controlspec = specs.OPAMP_A3, action = engine.opAmpA3}
  params:add{type = "control", id = "opAmpD3",name = "Osc3 Amp Env Decay", controlspec = specs.OPAMP_D3, action = engine.opAmpD3}
  params:add{type = "control", id = "opAmpS3",name = "Osc3 Amp Env Sustain", controlspec = specs.OPAMP_S3, action = engine.opAmpS3}
  params:add{type = "control", id = "opAmpR3",name = "Osc3 Amp Env Release", controlspec = specs.OPAMP_R3, action = engine.opAmpR3}
  params:add{type = "control", id = "opAmpA4",name = "Osc4 Amp Env Attack", controlspec = specs.OPAMP_A4, action = engine.opAmpA4}
  params:add{type = "control", id = "opAmpD4",name = "Osc4 Amp Env Decay", controlspec = specs.OPAMP_D4, action = engine.opAmpD4}
  params:add{type = "control", id = "opAmpS4",name = "Osc4 Amp Env Sustain", controlspec = specs.OPAMP_S4, action = engine.opAmpS4}
  params:add{type = "control", id = "opAmpR4",name = "Osc4 Amp Env Release", controlspec = specs.OPAMP_R4, action = engine.opAmpR4}
  params:add{type = "control", id = "opAmpA5",name = "Osc5 Amp Env Attack", controlspec = specs.OPAMP_A5, action = engine.opAmpA5}
  params:add{type = "control", id = "opAmpD5",name = "Osc5 Amp Env Decay", controlspec = specs.OPAMP_D5, action = engine.opAmpD5}
  params:add{type = "control", id = "opAmpS5",name = "Osc5 Amp Env Sustain", controlspec = specs.OPAMP_S5, action = engine.opAmpS5}
  params:add{type = "control", id = "opAmpR5",name = "Osc5 Amp Env Release", controlspec = specs.OPAMP_R5, action = engine.opAmpR5}
  params:add{type = "control", id = "opAmpA6",name = "Osc6 Amp Env Attack", controlspec = specs.OPAMP_A6, action = engine.opAmpA6}
  params:add{type = "control", id = "opAmpD6",name = "Osc6 Amp Env Decay", controlspec = specs.OPAMP_D6, action = engine.opAmpD6}
  params:add{type = "control", id = "opAmpS6",name = "Osc6 Amp Env Sustain", controlspec = specs.OPAMP_S6, action = engine.opAmpS6}
  params:add{type = "control", id = "opAmpR6",name = "Osc6 Amp Env Release", controlspec = specs.OPAMP_R6, action = engine.opAmpR6}
  params:bang()

end
return FM7