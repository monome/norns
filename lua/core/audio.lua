--- Audio class
-- @classmod audio
-- @alias Audio

local cs = require 'controlspec'

local Audio = {}

--- set headphone gain.
-- @tparam number gain (0-64)
Audio.headphone_gain = function(gain)
  _norns.gain_hp(gain)
end

--- set level for ADC input.
-- @param level in [0, 1]
Audio.level_adc = function(level)
  _norns.level_adc(level)
end

--- set level for *both* output channels.
-- @param level in [0, 1]
Audio.level_dac = function(level)
  _norns.level_dac(level)
end

-- @static
Audio.level_eng = function(level)
  _norns.level_ext(level)
end

--- set monitor level for *both* input channels.
-- @param level in [0, 1]
Audio.level_monitor = function(level)
  _norns.level_monitor(level)
end

--- set monitor mode to mono.
--- both inputs will be mixed to both outputs.
Audio.monitor_mono = function()
  _norns.monitor_mix_mono()
end

--- set monitor mode to stereo.
--- each input will be monitored on the respective output.
Audio.monitor_stereo = function()
  _norns.monitor_mix_stereo()
end

--- set tape level.
-- @param level [0,1]
Audio.level_tape = function(level)
  _norns.level_tape(level)
end

--- set cut master level.
-- @param level [0,1]
Audio.level_cut = function(level)
  _norns.level_cut_master(level)
end

--- enable input pitch analysis.
Audio.pitch_on = function()
  _norns.audio_pitch_on()
end

--- disable input pitch analysis (saves CPU).
Audio.pitch_off = function()
  _norns.audio_pitch_off()
end

--- restart the audio engine (recompile sclang).
Audio.restart = function()
   _norns.restart_audio()
end



--- Effects functions
-- @section Effects

--- reverb on.
function Audio.rev_on()
   _norns.rev_on()
end

--- reverb off.
function Audio.rev_off()
   _norns.rev_off()
end

--- reverb Monitor level.
-- @tparam number val
function Audio.level_monitor_rev(val)
   _norns.level_monitor_rev(val)
end

--- reverb ENGINE level.
-- @tparam number val
function Audio.level_eng_rev(val)
   _norns.level_ext_rev(val)
end

--- reverb TAPE level.
-- @tparam number val
function Audio.level_tape_rev(val)
   _norns.level_tape_rev(val)
end

--- reverb DAC level.
-- @tparam number val
function Audio.level_rev_dac(val)
   _norns.level_rev_dac(val)
end

--- set reverb parameter.
-- @tparam string name
-- @tparam number val
function Audio.rev_param(name, val)
   _norns.rev_param(name, val)
end

--- turn on compressor.
function Audio.comp_on()
   _norns.comp_on()
end

--- turn off compressor.
function Audio.comp_off()
   _norns.comp_off()
end

--- compressor mix amount.
-- @tparam number val
function Audio.comp_mix(val)
   _norns.comp_mix(val)
end

--- set compressor parameter.
-- @tparam string name
-- @tparam number val
function Audio.comp_param(name, val)
   _norns.comp_param(name, val)
end



--- Tape Functions
-- @section Tape

--- open a tape file.
-- @param file
Audio.tape_play_open = function(file)
  _norns.tape_play_open(file)
end

--- start tape playing.
Audio.tape_play_start = function()
  _norns.tape_play_start()
end

--- stop tape playing.
Audio.tape_play_stop = function()
  _norns.tape_play_stop()
end

--- open a tape recording file.
-- @param file
Audio.tape_record_open = function(file)
  _norns.tape_record_open(file)
end

--- start tape recording.
Audio.tape_record_start = function()
  _norns.tape_record_start()
end

--- stop tape recording.
Audio.tape_record_stop = function()
  _norns.tape_record_stop()
end


--- Softcut levels
-- @section softcut

--- softcut adc level.
-- @tparam number value
Audio.level_adc_cut = function(value)
  _norns.level_adc_cut(value)
end

--- softcut eng level.
-- @tparam number value
Audio.level_eng_cut = function(value)
  _norns.level_ext_cut(value)
end

--- softcut tape level.
-- @tparam number value
Audio.level_tape_cut = function(value)
  _norns.level_tape_cut(value)
end

--- softcut cut reverb level.
-- @tparam number value
Audio.level_cut_rev = function(value)
  _norns.level_cut_rev(value)
end


--- global functions
-- @section globals

--- callback for VU meters.
-- scripts should redefine this.
-- @param in1 input level 1 in [0, 63], audio taper
-- @param in2
-- @param out1
-- @param out2
Audio.vu = function(in1, in2, out1, out2)
   -- print (in1 .. '\t' .. in2 .. '\t' .. out1 .. '\t' .. out2)
end


--- helpers
-- @section helpers

--- set output level, clamped, save state.
-- @tparam number value audio level (0-64)
function Audio.set_audio_level(value)
  local l = util.clamp(value,0,64)
  if l ~= norns.state.out then
    norns.state.out = l
    Audio.output_level(l / 64.0)
  end
end

--- adjust output level, clamped, save state.
-- @tparam number delta amount to change output level
function Audio.adjust_output_level(delta)
  local l = util.clamp(norns.state.out + delta,0,64)
  if l ~= norns.state.out then
    norns.state.out = l
    Audio.output_level(l / 64.0)
  end
end

--- print audio file info 
-- @tparam string path (from dust directory)
function Audio.file_info(path)
  -- dur, ch, rate
  --print("file_info: " .. path)
  return _norns.sound_file_inspect(path)
end


function Audio.add_params()
  params:add_group("LEVELS",9)
  params:add_control("output_level", "output", 
    cs.new(-math.huge,0,'db',0,norns.state.mix.output,"dB"))
  params:set_action("output_level",
    function(x) 
      audio.level_dac(util.dbamp(x))
      norns.state.mix.output = x
    end)
  params:set_save("output_level", false)
  params:add_control("input_level", "input",
    cs.new(-math.huge,0,'db',0,norns.state.mix.input,"dB"))
  params:set_action("input_level",
    function(x)
      audio.level_adc(util.dbamp(x))
      norns.state.mix.input = x
    end)
  params:set_save("input_level", false)
  params:add_control("monitor_level", "monitor",
    cs.new(-math.huge,0,'db',0,norns.state.mix.monitor,"dB"))
  params:set_action("monitor_level",
    function(x)
      audio.level_monitor(util.dbamp(x))
      norns.state.mix.monitor = x
    end)
  params:set_save("monitor_level", false)
  params:add_control("engine_level", "engine",
    cs.new(-math.huge,0,'db',0,norns.state.mix.engine,"dB"))
  params:set_action("engine_level",
    function(x)
      audio.level_eng(util.dbamp(x))
      norns.state.mix.engine = x
    end)
  params:set_save("engine_level", false)
  params:add_control("softcut_level", "softcut",
    cs.new(-math.huge,0,'db',0,norns.state.mix.cut,"dB"))
  params:set_action("softcut_level",
    function(x)
      audio.level_cut(util.dbamp(x))
      norns.state.mix.cut = x
    end)
  params:set_save("softcut_level", false)
  params:add_control("tape_level", "tape",
    cs.new(-math.huge,0,'db',0,norns.state.mix.tape,"dB"))
  params:set_action("tape_level",
    function(x)
      audio.level_tape(util.dbamp(x))
      norns.state.mix.tape = x
    end)
  params:set_save("tape_level", false)
  params:add_separator()
  params:add_option("monitor_mode", "monitor mode", {"STEREO", "MONO"},
    norns.state.mix.monitor_mode)
  params:set_action("monitor_mode",
    function(x)
      if x == 1 then audio.monitor_stereo()
      else audio.monitor_mono() end
      norns.state.mix.monitor_mode = x
    end)
  params:set_save("monitor_mode", false)
  params:add_number("headphone_gain", "headphone gain", 0, 63,
    norns.state.mix.headphone_gain)
  params:set_action("headphone_gain",
    function(x)
      audio.headphone_gain(x)
      norns.state.mix.headphone_gain = x
    end)
  params:set_save("headphone_gain", false)
  
  params:add_group("REVERB",11)
  params:add_option("reverb", "reverb", {"OFF", "ON"}, 
    norns.state.mix.aux)
  params:set_action("reverb",
  function(x)
    if x == 1 then audio.rev_off() else audio.rev_on() end
    norns.state.mix.aux = x
  end)
  params:set_save("reverb", false)

  local cs_DB_LEVEL = cs.new(-math.huge,18,'db',0,0,"dB")
  local cs_DB_LEVEL_MUTE = cs.new(-math.huge,18,'db',0,-math.huge,"dB")
  local cs_DB_LEVEL_9DB = cs.new(-math.huge,18,'db',0,-9,"dB")

  params:add_control("rev_eng_input", "input engine", cs_DB_LEVEL_9DB)
  params:set_action("rev_eng_input",
  function(x) audio.level_eng_rev(util.dbamp(x)) end)

  params:add_control("rev_cut_input", "input softcut", cs_DB_LEVEL_9DB)
  params:set_action("rev_cut_input",
  function(x) audio.level_cut_rev(util.dbamp(x)) end)

  params:add_control("rev_monitor_input", "input monitor", cs_DB_LEVEL_MUTE)
  params:set_action("rev_monitor_input",
  function(x) audio.level_monitor_rev(util.dbamp(x)) end)

  params:add_control("rev_tape_input", "input tape", cs_DB_LEVEL_MUTE)
  params:set_action("rev_tape_input",
  function(x) audio.level_tape_rev(util.dbamp(x)) end)

  params:add_control("rev_return_level", "return level", cs_DB_LEVEL)
  params:set_action("rev_return_level",
  function(x) audio.level_rev_dac(util.dbamp(x)) end)


  local cs_IN_DELAY = cs.new(20,100,'lin',0,60,'ms')
  params:add_control("rev_pre_delay", "pre delay", cs_IN_DELAY)
  params:set_action("rev_pre_delay",
  function(x) audio.rev_param("pre_del",x) end)

  local cs_LF_FC = cs.new(50,1000,'exp',0, 200,'hz')
  params:add_control("rev_lf_fc", "lf fc", cs_LF_FC)
  params:set_action("rev_lf_fc",
  function(x) audio.rev_param("lf_fc",x) end)

  local cs_RT60 = cs.new(0.1,16,'lin',0,6,'s')
  params:add_control("rev_low_time", "low time", cs_RT60)
  params:set_action("rev_low_time",
  function(x) audio.rev_param("low_rt60",x) end)
  params:add_control("rev_mid_time", "mid time", cs_RT60)
  params:set_action("rev_mid_time",
  function(x) audio.rev_param("mid_rt60",x) end)

  local cs_HF_DAMP = cs.new(1500,20000,'exp',0,6000,'hz')
  params:add_control("rev_hf_damping", "hf damping", cs_HF_DAMP)
  params:set_action("rev_hf_damping",
  function(x) audio.rev_param("hf_damp",x) end)
  --[[
  local cs_EQ_FREQ1 = cs.new(40,2500,'exp',0,315,'hz')
  params:add_control("rev_eq1_freq","rev eq1 freq", cs_EQ_FREQ1)
  params:set_action("rev_eq1_freq",
  function(x) audio.aux_param("eq1_freq",x) end)
  local cs_EQ_LVL = cs.new(-15,15,'lin',0,0,"dB")
  params:add_control("rev_eq1_level","rev eq1 level", cs_EQ_LVL)
  params:set_action("rev_eq1_level",
  function(x) audio.aux_param("eq1_level",x) end)

  local cs_EQ_FREQ2 = cs.new(160,10000,'exp',0,1500,'hz')
  params:add_control("rev_eq2_freq","rev eq2 freq", cs_EQ_FREQ2)
  params:set_action("rev_eq2_freq",
  function(x) audio.aux_param("eq2_freq",x) end)
  params:add_control("rev_eq2_level","rev eq2 level", cs_EQ_LVL)
  params:set_action("rev_eq2_level",
  function(x) audio.aux_param("eq2_level",x) end)

  params:add_control("rev_level","rev level", cs_DB_LEVEL)
  params:set_action("rev_level",
  function(x) audio.aux_param("level",x) end)
  --]]

  params:add_group("COMPRESSOR",8)
  params:add_option("compressor", "compressor", {"OFF", "ON"},
    norns.state.mix.ins)
  params:set_action("compressor",
  function(x)
    if x == 1 then
      audio.comp_off()
    else
      audio.comp_on()
    end
    norns.state.mix.ins = x
  end)
  params:set_save("compressor", false)

  local cs_params = cs.new(0,1,'lin',0,1,'')
  params:add_control("comp_mix", "mix", cs_MIX)
  params:set_action("comp_mix",
  function(x) audio.comp_mix(x) end)

  local cs_RATIO = cs.new(1,20,'lin',0,4,'')
  params:add_control("comp_ratio", "ratio", cs_RATIO)
  params:set_action("comp_ratio",
  function(x) audio.comp_param("ratio",x) end)

  local cs_THRESH = cs.new(-100,10,'db',0,-18,'dB')
  params:add_control("comp_threshold", "threshold", cs_THRESH)
  params:set_action("comp_threshold",
  function(x) audio.comp_param("threshold",x) end)

  local cs_ATTACK = cs.new(1,1000,'exp',0,5,'ms')
  params:add_control("comp_attack", "attack", cs_ATTACK)
  params:set_action("comp_attack",
  function(x) audio.comp_param("attack", x*0.001) end)
  local cs_RELEASE = cs.new(1,1000,'exp',0,50,'ms')
  params:add_control("comp_release", "release", cs_RELEASE)
  params:set_action("comp_release",
  function(x) audio.comp_param("release",x * 0.001) end)

  local cs_PREGAIN = cs.new(-20,60,'db',0,0,'dB')
  params:add_control("comp_pre_gain", "pre gain", cs_PREGAIN)
  params:set_action("comp_pre_gain",
  function(x) audio.comp_param("gain_pre",x) end)

  local cs_POSTGAIN = cs.new(-20,60,'db',0,9,'dB')
  params:add_control("comp_post_gain", "post gain", cs_POSTGAIN)
  params:set_action("comp_post_gain",
  function(x) audio.comp_param("gain_post",x) end)


  params:add_group("SOFTCUT",3)

  params:add_control("cut_input_adc", "input adc", cs_MAIN_LEVEL)
  params:set_action("cut_input_adc",
    function(x) audio.level_adc_cut(util.dbamp(x)) end)

  params:add_control("cut_input_eng", "input engine", cs_MAIN_LEVEL)
  params:set_action("cut_input_eng",
    function(x) audio.level_eng_cut(util.dbamp(x)) end)

  params:add_control("cut_input_tape", "input tape", cs_MAIN_LEVEL)
  params:set_action("cut_input_tape",
    function(x) audio.level_tape_cut(util.dbamp(x)) end)

end

return Audio
