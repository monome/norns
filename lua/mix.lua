local cs = require 'controlspec'
local fx = require 'effects'

-- mix paramset
local mix = paramset.new("mix", "mix")
local cs_MAIN_LEVEL = cs.new(-math.huge,0,'db',0,0,"dB")
mix:add_control("output", "output", cs_MAIN_LEVEL)
mix:set_action("output",
  function(x) norns.audio.output_level(x) end)
mix:add_control("input", "input", cs_MAIN_LEVEL)
mix:set_action("input",
  function(x)
    norns.audio.input_level(1,x)
    norns.audio.input_level(2,x)
  end)
local cs_MUTE_LEVEL = cs.new(-math.huge,0,'db',0,-math.huge,"dB")
mix:add_control("monitor", "monitor", cs_MUTE_LEVEL)
mix:set_action("monitor",
  function(x) norns.audio.monitor_level(x) end)
mix:add_option("monitor_mode", "monitor mode", {"STEREO", "MONO"})
mix:set_action("monitor_mode",
  function(x)
    if x == 1 then norns.audio.monitor_stereo()
    else norns.audio.monitor_mono() end
  end)
mix:add_number("headphone", "headphone", 0, 63, 40)
mix:set_action("headphone",
  function(x) gain_hp(x) end)

-- TODO TAPE (rec) modes: OUTPUT, OUTPUT+MONITOR, OUTPUT/MONITOR SPLIT
-- TODO TAPE (playback) VOL, SPEED?

-- ControlSpec.new(minval, maxval, warp, step, default, units)
mix:add_separator()
mix:add_option("aux_fx", "aux fx", {"OFF", "ON"}, 2)
mix:set_action("aux_fx",
  function(x)
    if x == 1 then
      fx.aux_fx_off()
    else
      fx.aux_fx_on()
    end
  end)
local cs_DB_LEVEL = cs.new(-math.huge,18,'db',0,0,"dB")
local cs_DB_LEVEL_MUTE = cs.new(-math.huge,18,'db',0,-math.huge,"dB")
local cs_DB_LEVEL_9DB = cs.new(-math.huge,18,'db',0,-9,"dB")
mix:add_control("aux_engine_level", "aux engine level", cs_DB_LEVEL_9DB)
mix:set_action("aux_engine_level",
  function(x) fx.aux_fx_output_level(x) end)
mix:add_control("aux_input1_level", "aux input1 level", cs_DB_LEVEL_MUTE)
mix:set_action("aux_input1_level",
  function(x) fx.aux_fx_input_level(1,x) end)
mix:add_control("aux_input2_level", "aux input2 level", cs_DB_LEVEL_MUTE)
mix:set_action("aux_input2_level",
  function(x) fx.aux_fx_input_level(2,x) end)
mix:add_control("aux_input1_pan", "aux input1 pan", cs_PAN)
mix:set_action("aux_input1_pan",
  function(x) fx.aux_fx_input_pan(1,x) end)
mix:add_control("aux_input2_pan", "aux input2 pan", cs_PAN)
mix:set_action("aux_input2_pan",
  function(x) fx.aux_fx_input_pan(2,x) end)
mix:add_control("aux_return_level", "aux return level", cs_DB_LEVEL)
mix:set_action("aux_return_level",
  function(x) fx.aux_fx_return_level(x) end)


local cs_IN_DELAY = cs.new(20,100,'lin',0,60,'ms')
mix:add_control("rev_pre_delay", "rev pre delay", cs_IN_DELAY)
mix:set_action("rev_pre_delay",
  function(x) fx.aux_fx_param("in_delay",x) end)

local cs_LF_X = cs.new(50,1000,'exp',0,200,'hz')
mix:add_control("rev_lf_x", "rev lf x", cs_LF_X)
mix:set_action("rev_lf_x",
  function(x) fx.aux_fx_param("lf_x",x) end)

local cs_RT60 = cs.new(0.1,8,'lin',0,6,'s')
mix:add_control("rev_low_time", "rev low time", cs_RT60)
mix:set_action("rev_low_time",
  function(x) fx.aux_fx_param("low_rt60",x) end)
mix:add_control("rev_mid_time", "rev mid time", cs_RT60)
mix:set_action("rev_mid_time",
  function(x) fx.aux_fx_param("mid_rt60",x) end)

local cs_HF_DAMP = cs.new(1500,20000,'exp',0,6000,'hz')
mix:add_control("rev_hf_damping", "rev hf damping", cs_HF_DAMP)
mix:set_action("rev_hf_damping",
  function(x) fx.aux_fx_param("hf_damping",x) end)

local cs_EQ_FREQ1 = cs.new(40,2500,'exp',0,315,'hz')
mix:add_control("rev_eq1_freq","rev eq1 freq", cs_EQ_FREQ1)
mix:set_action("rev_eq1_freq",
  function(x) fx.aux_fx_param("eq1_freq",x) end)
local cs_EQ_LVL = cs.new(-15,15,'lin',0,0,"dB")
mix:add_control("rev_eq1_level","rev eq1 level", cs_EQ_LVL)
mix:set_action("rev_eq1_level",
  function(x) fx.aux_fx_param("eq1_level",x) end)

local cs_EQ_FREQ2 = cs.new(160,10000,'exp',0,1500,'hz')
mix:add_control("rev_eq2_freq","rev eq2 freq", cs_EQ_FREQ2)
mix:set_action("rev_eq2_freq",
  function(x) fx.aux_fx_param("eq2_freq",x) end)
mix:add_control("rev_eq2_level","rev eq2 level", cs_EQ_LVL)
mix:set_action("rev_eq2_level",
  function(x) fx.aux_fx_param("eq2_level",x) end)

local cs_LEVEL = cs.new(-70,40,'lin',0,0,'dB')
mix:add_control("rev_level","rev level", cs_LEVEL)
mix:set_action("rev_level",
  function(x) fx.aux_fx_param("level",x) end)


mix:add_separator()
mix:add_option("insert_fx", "insert fx", {"OFF", "ON"})
mix:set_action("insert_fx",
  function(x)
    if x == 1 then
      fx.insert_fx_off()
    else
      fx.insert_fx_on()
    end
  end)
local cs_MIX = cs.new(0,1,'lin',0,1,'')
mix:add_control("insert_mix", "insert mix", cs_MIX)
mix:set_action("insert_mix",
  function(x) fx.insert_fx_mix(x) end)

local cs_RATIO = cs.new(1,20,'lin',0,4,'')
mix:add_control("comp_ratio", "comp ratio", cs_RATIO)
mix:set_action("comp_ratio",
  function(x) fx.insert_fx_param("level",x) end)

local cs_THRESH = cs.new(-100,10,'db',0,-18,'dB')
mix:add_control("comp_threshold", "comp threshold", cs_THRESH)
mix:set_action("comp_threshold",
  function(x) fx.insert_fx_param("threshold",x) end)

local cs_ATTACK = cs.new(1,1000,'exp',0,5,'ms')
mix:add_control("comp_attack", "comp attack", cs_ATTACK)
mix:set_action("comp_attack",
  function(x) fx.insert_fx_param("attack",x) end)
local cs_RELEASE = cs.new(1,1000,'exp',0,50,'ms')
mix:add_control("comp_release", "comp release", cs_RELEASE)
mix:set_action("comp_release",
  function(x) fx.insert_fx_param("release",x) end)

local cs_MAKEUP = cs.new(-20,60,'db',0,9,'dB')
mix:add_control("comp_makeup_gain", "comp makeup gain", cs_MAKEUP)
mix:set_action("comp_makeup_gain",
  function(x) fx.insert_fx_param("makeup_gain",x) end)

return mix
