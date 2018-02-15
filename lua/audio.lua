--- Audio class
-- @module audio
-- @alias Audio

require 'norns'
norns.version.audio = '0.0.2'

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
Audio.restart = function() restart_audio() end

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

norns.vu = function(in1, in2, out1, out2)
   -- anything else to do?
   Audio.vu(in1, in2, out1, out2)
end

return Audio
