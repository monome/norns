--- Audio class
--
-- The [norns script reference](https://monome.org/docs/norns/reference/)
-- has [examples for this module](https://monome.org/docs/norns/reference/audio).
--
-- @module audio
-- @alias Audio

local cs = require 'controlspec'
local Observable = require 'lib/container/observable'
local tab = require 'tabutil'

local Audio = {}

--
-- Tape state
--

-- play state constants
local TAPE_PLAY_STATE_EMPTY = 0
local TAPE_PLAY_STATE_READY = 1
local TAPE_PLAY_STATE_PLAYING = 2
local TAPE_PLAY_STATE_PAUSED = 3

-- record state constants
local TAPE_REC_STATE_EMPTY = 0
local TAPE_REC_STATE_READY = 1
local TAPE_REC_STATE_RECORDING = 2
local TAPE_REC_STATE_PAUSED = 3

-- default reserve seconds when computing diskfree
local TAPE_DISK_RESERVE = 250

-- underlying observable tape state; observers receive the full state table on each update
local tape_state = Observable.new({
  play = { state = TAPE_PLAY_STATE_EMPTY, pos = 0.0, len = 0.0, loop = true, file = nil },
  rec  = { state = TAPE_REC_STATE_EMPTY, pos = 0.0, file = nil },
})

Audio.tape = {}

--- get current tape state value
-- Returns the same structure provided to subscribers.
function Audio.tape.get_state()
  return tape_state:value()
end

--- subscribe to tape state updates.
-- @tparam any key unique key for this subscription (usually a table)
-- @tparam function fn callback receiving full snapshot table
function Audio.tape.subscribe(key, fn)
  return tape_state:register(key, fn)
end

--- unsubscribe from tape state updates.
-- @tparam any key unique key used when subscribing
function Audio.tape.unsubscribe(key)
  return tape_state:unregister(key)
end

-- engine callback handlers
function Audio.tape._on_status(play_state, play_pos_s, play_len_s, rec_state, rec_pos_s, loop_enabled)
  local current_state = Audio.tape.get_state()

  local next_play = tab.gather(current_state.play, {})
  next_play.state = play_state
  next_play.pos = play_pos_s
  next_play.len = play_len_s
  next_play.loop = (loop_enabled ~= 0)

  local next_rec = tab.gather(current_state.rec, {})
  next_rec.state = rec_state
  next_rec.pos = rec_pos_s

  local next_state = tab.gather(current_state, {})
  next_state.play = next_play
  next_state.rec = next_rec

  tape_state:set(next_state)
end

function Audio.tape._on_play_file(path)
  local current_state = Audio.tape.get_state()
  local filename = nil
  if type(path) == 'string' then
    filename = path:match("([^/\\]+)$")
  end
  local next_play = tab.gather(current_state.play, {})
  next_play.file = filename
  local updated_state = tab.gather(current_state, {
    play = next_play
  })
  tape_state:set(updated_state)
end

function Audio.tape._on_rec_file(path)
  local current_state = Audio.tape.get_state()
  local filename = nil
  if type(path) == 'string' then
    filename = path:match("([^/\\]+)$")
  end
  local next_rec = tab.gather(current_state.rec, {})
  next_rec.file = filename
  local updated_state = tab.gather(current_state, {
    rec = next_rec
  })
  tape_state:set(updated_state)
end

-- state constants for consumers
Audio.tape.constants = {
  TAPE_PLAY_STATE_EMPTY = TAPE_PLAY_STATE_EMPTY,
  TAPE_PLAY_STATE_READY = TAPE_PLAY_STATE_READY,
  TAPE_PLAY_STATE_PLAYING = TAPE_PLAY_STATE_PLAYING,
  TAPE_PLAY_STATE_PAUSED = TAPE_PLAY_STATE_PAUSED,
  TAPE_REC_STATE_EMPTY = TAPE_REC_STATE_EMPTY,
  TAPE_REC_STATE_READY = TAPE_REC_STATE_READY,
  TAPE_REC_STATE_RECORDING = TAPE_REC_STATE_RECORDING,
  TAPE_REC_STATE_PAUSED = TAPE_REC_STATE_PAUSED,
}

-- tape state getters
function Audio.tape_is_recording() return Audio.tape.get_state().rec.state == TAPE_REC_STATE_RECORDING end

--- compute diskfree seconds at 48k/16-bit stereo
-- reserve a small buffer by subtracting `reserve_s` seconds from `norns.disk`
-- @tparam number reserve_s seconds to reserve
-- @treturn number whole seconds available for new recording
function Audio.tape_compute_diskfree(reserve_s)
  local reserve = reserve_s or TAPE_DISK_RESERVE
  if norns and norns.disk then
    return math.floor((norns.disk - reserve) / .192)
  end
  return 0
end

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

--- pause or resume tape playback.
-- @tparam boolean paused
Audio.tape_play_pause = function(paused)
  _norns.tape_play_pause(paused and 1 or 0)
end

--- stop tape playing.
Audio.tape_play_stop = function()
  _norns.tape_play_stop()
end

--- set tape looping on/off.
-- @tparam boolean enabled
Audio.tape_play_loop = function(enabled)
  _norns.tape_play_loop(enabled and 1 or 0)
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

--- pause or resume tape recording.
-- @tparam boolean paused
Audio.tape_record_pause = function(paused)
  _norns.tape_record_pause(paused and 1 or 0)
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
-- @treturn integer number of audio channels
-- @treturn integer number of samples
-- @treturn integer sample rate
function Audio.file_info(path)
  -- ch, samples, rate
  --print("file_info: " .. path)
  return _norns.sound_file_inspect(path)
end


function Audio.add_params()
  params:add_group("LEVELS",9)
  params:add_control("output_level", "output", 
    cs.new(-math.huge,6,'db',0,norns.state.mix.output,"dB"))
  params:set_action("output_level",
    function(x) 
      audio.level_dac(util.dbamp(x))
      norns.state.mix.output = x
    end)
  params:set_save("output_level", false)
  params:add_control("input_level", "input",
    cs.new(-math.huge,6,'db',0,norns.state.mix.input,"dB"))
  params:set_action("input_level",
    function(x)
      audio.level_adc(util.dbamp(x))
      norns.state.mix.input = x
    end)
  params:set_save("input_level", false)
  params:add_control("monitor_level", "monitor",
    cs.new(-math.huge,6,'db',0,norns.state.mix.monitor,"dB"))
  params:set_action("monitor_level",
    function(x)
      audio.level_monitor(util.dbamp(x))
      norns.state.mix.monitor = x
    end)
  params:set_save("monitor_level", false)
  params:add_control("engine_level", "engine",
    cs.new(-math.huge,6,'db',0,norns.state.mix.engine,"dB"))
  params:set_action("engine_level",
    function(x)
      audio.level_eng(util.dbamp(x))
      norns.state.mix.engine = x
    end)
  params:set_save("engine_level", false)
  params:add_control("softcut_level", "softcut",
    cs.new(-math.huge,6,'db',0,norns.state.mix.cut,"dB"))
  params:set_action("softcut_level",
    function(x)
      audio.level_cut(util.dbamp(x))
      norns.state.mix.cut = x
    end)
  params:set_save("softcut_level", false)
  params:add_control("tape_level", "tape",
    cs.new(-math.huge,6,'db',0,norns.state.mix.tape,"dB"))
  params:set_action("tape_level",
    function(x)
      audio.level_tape(util.dbamp(x))
      norns.state.mix.tape = x
    end)
  params:set_save("tape_level", false)
  params:add_separator("monitoring_separator", "monitoring")
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

  --local cs_DB_LEVEL = cs.new(-math.huge,18,'db',0,0,"dB")
  --local cs_DB_LEVEL_MUTE = cs.new(-math.huge,18,'db',0,-math.huge,"dB")
  --local cs_DB_LEVEL_9DB = cs.new(-math.huge,18,'db',0,-9,"dB")

  params:add_control("rev_eng_input", "input engine",
    cs.new(-math.huge,18,'db',0,norns.state.mix.rev_eng_input,"dB"))
  params:set_action("rev_eng_input",
  function(x)
    audio.level_eng_rev(util.dbamp(x))
    norns.state.mix.rev_eng_input = x
  end)

  params:add_control("rev_cut_input", "input softcut", 
    cs.new(-math.huge,18,'db',0,norns.state.mix.rev_eng_input,"dB"))
  params:set_action("rev_cut_input",
  function(x)
    audio.level_cut_rev(util.dbamp(x))
    norns.state.mix.rev_cut_input = x
  end)

  params:add_control("rev_monitor_input", "input monitor",
    cs.new(-math.huge,18,'db',0,norns.state.mix.rev_monitor_input,"dB"))
  params:set_action("rev_monitor_input",
  function(x)
    audio.level_monitor_rev(util.dbamp(x))
    norns.state.mix.rev_monitor_input = x
  end)

  params:add_control("rev_tape_input", "input tape",
    cs.new(-math.huge,18,'db',0,norns.state.mix.rev_tape_input,"dB"))
  params:set_action("rev_tape_input",
  function(x)
    audio.level_tape_rev(util.dbamp(x))
    norns.state.mix.rev_tape_input = x
  end)

  params:add_control("rev_return_level", "return level",
    cs.new(-math.huge,18,'db',0,norns.state.mix.rev_return_level,"dB"))
  params:set_action("rev_return_level",
  function(x)
    audio.level_rev_dac(util.dbamp(x))
    norns.state.mix.rev_return_level = x
  end)

  params:add_control("rev_pre_delay", "pre delay",
    cs.new(20,100,'lin',0,norns.state.mix.rev_pre_delay,'ms'))
  params:set_action("rev_pre_delay",
  function(x)
    audio.rev_param("pre_del",x)
    norns.state.mix.rev_pre_delay = x
  end)

  params:add_control("rev_lf_fc", "lf fc",
    cs.new(50,1000,'exp',0, norns.state.mix.rev_lf_fc,'hz'))
  params:set_action("rev_lf_fc",
  function(x)
    audio.rev_param("lf_fc",x)
    norns.state.mix.rev_lf_fc = x
  end)

  params:add_control("rev_low_time", "low time",
    cs.new(0.1,16,'lin',0,norns.state.mix.rev_low_time,'s'))
  params:set_action("rev_low_time",
  function(x)
    audio.rev_param("low_rt60",x)
    norns.state.mix.rev_low_time = x
  end)

  params:add_control("rev_mid_time", "mid time",
    cs.new(0.1,16,'lin',0,norns.state.mix.rev_mid_time,'s'))
  params:set_action("rev_mid_time",
  function(x)
    audio.rev_param("mid_rt60",x)
    norns.state.mix.rev_mid_time = x
  end)

  params:add_control("rev_hf_damping", "hf damping", 
    cs.new(1500,20000,'exp',0,norns.state.mix.rev_hf_damping,'hz'))
  params:set_action("rev_hf_damping",
  function(x)
    audio.rev_param("hf_damp",x)
    norns.state.mix.rev_hf_damping = x
  end)

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

  params:add_control("comp_mix", "mix",
    cs.new(0,1,'lin',0,norns.state.mix.comp_mix,''))
  params:set_action("comp_mix",
  function(x)
    audio.comp_mix(x)
    norns.state.mix.comp_mix = x
  end)

  params:add_control("comp_ratio", "ratio",
    cs.new(1,20,'lin',0,norns.state.mix.comp_ratio,''))
  params:set_action("comp_ratio",
  function(x)
    audio.comp_param("ratio",x)
    norns.state.mix.comp_ratio = x
  end)

  params:add_control("comp_threshold", "threshold",
    cs.new(-100,10,'db',0,norns.state.mix.comp_threshold,'dB'))
    params:set_action("comp_threshold",
  function(x)
    audio.comp_param("threshold",x)
    norns.state.mix.comp_threshold = x
  end)

  params:add_control("comp_attack", "attack",
    cs.new(1,1000,'exp',0,norns.state.mix.comp_attack,'ms'))
  params:set_action("comp_attack",
  function(x)
    audio.comp_param("attack", x*0.001)
    norns.state.mix.comp_attack = x
  end)

  
  params:add_control("comp_release", "release",
    cs.new(1,1000,'exp',0,norns.state.mix.comp_release,'ms'))
  params:set_action("comp_release",
  function(x)
    audio.comp_param("release",x * 0.001)
    norns.state.mix.comp_release = x
  end)

  params:add_control("comp_pre_gain", "pre gain",
    cs.new(-20,60,'db',0,norns.state.mix.comp_pre_gain,'dB'))
  params:set_action("comp_pre_gain",
  function(x)
    audio.comp_param("gain_pre",x)
    norns.state.mix.comp_pre_gain = x
  end)

  params:add_control("comp_post_gain", "post gain",
    cs.new(-20,60,'db',0,norns.state.mix.comp_post_gain,'dB'))
  params:set_action("comp_post_gain",
  function(x)
    audio.comp_param("gain_post",x)
    norns.state.mix.comp_post_gain = x
  end)


  params:add_group("SOFTCUT",3)

  params:add_control("cut_input_adc", "input adc",
    cs.new(-math.huge,18,'db',0,norns.state.mix.cut_input_adc,"dB"))
  params:set_action("cut_input_adc",
    function(x)
      audio.level_adc_cut(util.dbamp(x))
      norns.state.mix.cut_input_adc = x
    end)

  params:add_control("cut_input_eng", "input engine",
    cs.new(-math.huge,18,'db',0,norns.state.mix.cut_input_eng,"dB"))
  params:set_action("cut_input_eng",
    function(x)
      audio.level_eng_cut(util.dbamp(x))
      norns.state.mix.cut_input_eng = x
    end)

  params:add_control("cut_input_tape", "input tape",
    cs.new(-math.huge,18,'db',0,norns.state.mix.cut_input_tape,"dB"))
  params:set_action("cut_input_tape",
    function(x)
      audio.level_tape_cut(util.dbamp(x))
      norns.state.mix.cut_input_tape = x
    end)

end

return Audio
