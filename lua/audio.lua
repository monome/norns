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


return Audio
