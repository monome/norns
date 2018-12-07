--- Audio class
-- @module audio
-- @alias Audio

local Audio = {}

--- set level for one input channel
-- @param channel (1 or 2)
-- @param level in [0, 1]
Audio.input_level = function(channel, level)
  audio_input_level(channel, level)
end

--- set level for *both* output channels
-- @param level in [0, 1]
Audio.output_level = function(level)
  audio_output_level(level)
end

-- set monitor level for *both* input channels
-- @param level in [0, 1]
Audio.monitor_level = function(level)
  audio_monitor_level(level)
end

--- set monitor mode to mono
--- both inputs will be mixed to both outputs
Audio.monitor_mono = function()
  audio_monitor_mono()
end

--- set monitor mode to stereo
--- each input will be monitored on the respective output
Audio.monitor_stereo = function()
  audio_monitor_stereo()
end

--- enable monitoring (may cause clicks)
Audio.monitor_on = function()
  audio_monitor_on()
end

--- disable monitoring (may cause clicks)
Audio.monitor_off = function()
  audio_monitor_off()
end

--- enable input pitch analysis
Audio.pitch_on = function()
  audio_pitch_on()
end

--- disable input pitch analysis (saves CPU)
Audio.pitch_off = function()
  audio_pitch_off()
end

--- restart the audio engine (recompile sclang)
Audio.restart = function()
   restart_audio()
end

--- global functions
-- @section globals

--- callback for VU meters
--- scripts should redefine this
-- @param in1 input level 1 in [0, 63], audio taper
-- @param in2
-- @param out1
-- @param out2
Audio.vu = function(in1, in2, out1, out2)
   -- print (in1 .. '\t' .. in2 .. '\t' .. out1 .. '\t' .. out2)
end


--- helpers
-- @section helpers

--- set output level, clamped, save state
-- @param value audio level (0-64)
function Audio.set_audio_level(value)
  local l = util.clamp(value,0,64)
  if l ~= norns.state.out then
    norns.state.out = l
    Audio.output_level(l / 64.0)
  end
end

--- adjust output level, clamped, save state
-- @param delta amount to change output level
function Audio.adjust_output_level(delta)
  local l = util.clamp(norns.state.out + delta,0,64)
  if l ~= norns.state.out then
    norns.state.out = l
    Audio.output_level(l / 64.0)
  end
end


return Audio
