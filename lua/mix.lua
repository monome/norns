local cs = require 'controlspec'
local util = require 'util'
local audio = require 'audio'

-- mix paramset
local mix = paramset.new("mix", "mix")
local cs_MAIN_LEVEL = cs.new(-math.huge,0,'db',0,0,"dB")
mix:add_control("output", "output", cs_MAIN_LEVEL)
mix:set_action("output",
  function(x) audio.output_level(util.dbamp(x)) end)
mix:add_control("input", "input", cs_MAIN_LEVEL)
mix:set_action("input",
  function(x)
    audio.input_level(1,util.dbamp(x))
    audio.input_level(2,util.dbamp(x))
  end)
local cs_MUTE_LEVEL = cs.new(-math.huge,0,'db',0,-math.huge,"dB")
mix:add_control("monitor", "monitor", cs_MUTE_LEVEL)
mix:set_action("monitor",
  function(x) audio.monitor_level(util.dbamp(x)) end)
mix:add_option("monitor_mode", "monitor mode", {"STEREO", "MONO"})
mix:set_action("monitor_mode",
  function(x)
    if x == 1 then audio.monitor_stereo()
    else audio.monitor_mono() end
  end)
mix:add_control("tape", "tape", cs_MUTE_LEVEL)
mix:set_action("tape",
  function(x) audio.level_tape(util.dbamp(x)) end)
mix:add_number("headphone", "headphone", 0, 63, 40)
mix:set_action("headphone",
  function(x) audio.headphone_gain(x) end)


-- TODO TAPE (rec) modes: OUTPUT, OUTPUT+MONITOR, OUTPUT/MONITOR SPLIT
-- TODO TAPE (playback) VOL, SPEED?

-- ControlSpec.new(minval, maxval, warp, step, default, units)
mix:add_separator()
mix:add_option("aux", "aux", {"OFF", "ON"}, 2)
mix:set_action("aux",
  function(x)
    if x == 1 then
      audio.aux_off()
    else
      audio.aux_on()
    end
  end)
local cs_DB_LEVEL = cs.new(-math.huge,18,'db',0,0,"dB")
local cs_DB_LEVEL_MUTE = cs.new(-math.huge,18,'db',0,-math.huge,"dB")
local cs_DB_LEVEL_9DB = cs.new(-math.huge,18,'db',0,-9,"dB")

mix:add_control("aux_engine_level", "aux engine level", cs_DB_LEVEL_9DB)
mix:set_action("aux_engine_level",
  function(x) audio.level_ext_aux(x) end)

mix:add_control("aux_monitor_level", "aux monitor level", cs_DB_LEVEL_MUTE)
mix:set_action("aux_monitor_level",
  function(x) audio.level_monitor_aux(0,x) end)

mix:add_control("aux_return_level", "aux return level", cs_DB_LEVEL)
mix:set_action("aux_return_level",
  function(x) audio.level_aux_dac(x) end)


local cs_IN_DELAY = cs.new(20,100,'lin',0,60,'ms')
mix:add_control("rev_pre_delay", "rev pre delay", cs_IN_DELAY)
mix:set_action("rev_pre_delay",
  function(x) audio.aux_param("in_delay",x) end)

local cs_LF_FC = cs.new(50,1000,'exp',0, 200,'hz')
mix:add_control("rev_lf_fc", "rev lf fc", cs_LF_FC)
mix:set_action("rev_lf_fc",
  function(x) audio.aux_param("lf_fc",x) end)

local cs_RT60 = cs.new(0.1,8,'lin',0,6,'s')
mix:add_control("rev_low_time", "rev low time", cs_RT60)
mix:set_action("rev_low_time",
  function(x) audio.aux_param("low_rt60",x) end)
mix:add_control("rev_mid_time", "rev mid time", cs_RT60)
mix:set_action("rev_mid_time",
  function(x) audio.aux_param("mid_rt60",x) end)

local cs_HF_DAMP = cs.new(1500,20000,'exp',0,6000,'hz')
mix:add_control("rev_hf_damping", "rev hf damping", cs_HF_DAMP)
mix:set_action("rev_hf_damping",
  function(x) audio.aux_param("hf_damping",x) end)

local cs_EQ_FREQ1 = cs.new(40,2500,'exp',0,315,'hz')
mix:add_control("rev_eq1_freq","rev eq1 freq", cs_EQ_FREQ1)
mix:set_action("rev_eq1_freq",
  function(x) audio.aux_param("eq1_freq",x) end)
local cs_EQ_LVL = cs.new(-15,15,'lin',0,0,"dB")
mix:add_control("rev_eq1_level","rev eq1 level", cs_EQ_LVL)
mix:set_action("rev_eq1_level",
  function(x) audio.aux_param("eq1_level",x) end)

local cs_EQ_FREQ2 = cs.new(160,10000,'exp',0,1500,'hz')
mix:add_control("rev_eq2_freq","rev eq2 freq", cs_EQ_FREQ2)
mix:set_action("rev_eq2_freq",
  function(x) audio.aux_param("eq2_freq",x) end)
mix:add_control("rev_eq2_level","rev eq2 level", cs_EQ_LVL)
mix:set_action("rev_eq2_level",
  function(x) audio.aux_param("eq2_level",x) end)

mix:add_control("rev_level","rev level", cs_DB_LEVEL)
mix:set_action("rev_level",
  function(x) audio.aux_param("level",x) end)


mix:add_separator()
mix:add_option("insert", "insert", {"OFF", "ON"})
mix:set_action("insert",
  function(x)
    if x == 1 then
      audio.insert_off()
    else
      audio.insert_on()
    end
  end)
local cs_MIX = cs.new(0,1,'lin',0,1,'')
mix:add_control("insert_mix", "insert mix", cs_MIX)
mix:set_action("insert_mix",
  function(x) audio.insert_mix(x) end)

local cs_RATIO = cs.new(1,20,'lin',0,4,'')
mix:add_control("comp_ratio", "comp ratio", cs_RATIO)
mix:set_action("comp_ratio",
  function(x) audio.insert_param("level",x) end)

local cs_THRESH = cs.new(-100,10,'db',0,-18,'dB')
mix:add_control("comp_threshold", "comp threshold", cs_THRESH)
mix:set_action("comp_threshold",
  function(x) audio.insert_param("threshold",x) end)

local cs_ATTACK = cs.new(1,1000,'exp',0,5,'ms')
mix:add_control("comp_attack", "comp attack", cs_ATTACK)
mix:set_action("comp_attack",
  function(x) audio.insert_param("attack",x) end)
local cs_RELEASE = cs.new(1,1000,'exp',0,50,'ms')
mix:add_control("comp_release", "comp release", cs_RELEASE)
mix:set_action("comp_release",
  function(x) audio.insert_param("release",x) end)

local cs_MAKEUP = cs.new(-20,60,'db',0,9,'dB')
mix:add_control("comp_makeup_gain", "comp makeup gain", cs_MAKEUP)
mix:set_action("comp_makeup_gain",
  function(x) audio.insert_param("makeup_gain",x) end)

return mix
