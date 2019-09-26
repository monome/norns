--- Audio class
-- @module audio
-- @alias Audio

local Audio = {}

--- set headphone gain.
-- @param gain (0-64)
Audio.headphone_gain = function(gain)
  gain_hp(gain)
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
  audio_pitch_on()
end

--- disable input pitch analysis (saves CPU).
Audio.pitch_off = function()
  audio_pitch_off()
end

--- restart the audio engine (recompile sclang).
Audio.restart = function()
   restart_audio()
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
-- @param val
function Audio.level_monitor_rev(val)
   _norns.level_monitor_rev(val)
end

--- reverb ENGINE level.
-- @param val
function Audio.level_eng_rev(val)
   _norns.level_ext_rev(val)
end

--- reverb TAPE level.
-- @param val
function Audio.level_tape_rev(val)
   _norns.level_tape_rev(val)
end

--- reverb DAC level.
-- @param val
function Audio.level_rev_dac(val)
   _norns.level_rev_dac(val)
end

--- set reverb parameter.
-- @param name
-- @param val
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
-- @param val
function Audio.comp_mix(val)
   _norns.comp_mix(val)
end

--- set compressor parameter.
-- @param name
-- @param val
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
-- @param value
Audio.level_adc_cut = function(value)
  _norns.level_adc_cut(value)
end

--- softcut eng level.
-- @param value
Audio.level_eng_cut = function(value)
  _norns.level_ext_cut(value)
end

--- softcut tape level.
-- @param value
Audio.level_tape_cut = function(value)
  _norns.level_tape_cut(value)
end

--- softcut cut reverb level.
-- @param value
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
-- @param value audio level (0-64)
function Audio.set_audio_level(value)
  local l = util.clamp(value,0,64)
  if l ~= norns.state.out then
    norns.state.out = l
    Audio.output_level(l / 64.0)
  end
end

--- adjust output level, clamped, save state.
-- @param delta amount to change output level
function Audio.adjust_output_level(delta)
  local l = util.clamp(norns.state.out + delta,0,64)
  if l ~= norns.state.out then
    norns.state.out = l
    Audio.output_level(l / 64.0)
  end
end

--- print audio file info 
-- @param path (from dust directory)
function Audio.file_info(path)
  -- dur, ch, rate
  --print("file_info: " .. path)
  return sound_file_inspect(path)
end


return Audio
