--- Audio class
-- @module audio
-- @alias Audio

local Audio = {}

--- set headphone gain
-- @param gain (0-64)
Audio.headphone_gain = function(gain)
  gain_hp(gain)
end

--- set level for ADC input
-- @param level in [0, 1]
Audio.level_adc = function(level)
  _norns.level_adc(level)
end

--- set level for *both* output channels
-- @param level in [0, 1]
Audio.level_dac = function(level)
  _norns.level_dac(level)
end

Audio.level_ext = function(level)
  _norns.level_ext(level)
end

-- set monitor level for *both* input channels
-- @param level in [0, 1]
Audio.level_monitor = function(level)
  _norns.level_monitor(level)
end

--- set monitor mode to mono
--- both inputs will be mixed to both outputs
Audio.monitor_mono = function()
  _norns.monitor_mix_mono()
end

--- set monitor mode to stereo
--- each input will be monitored on the respective output
Audio.monitor_stereo = function()
  _norns.monitor_mix_stereo()
end

--- set tape level
-- @param level [0,1]
Audio.level_tape = function(level)
  _norns.level_tape(level)
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




-- EFFECTS

function Audio.rev_on()
   _norns.rev_on()
end

function Audio.rev_off()
   _norns.rev_off()
end

function Audio.level_monitor_rev(val)
   _norns.level_monitor_rev(val)
end

function Audio.level_ext_rev(val)
   _norns.level_ext_rev(val)
end

function Audio.level_rev_dac(val)
   _norns.level_rev_dac(val)
end

function Audio.rev_param(name, val)
   _norns.rev_param(name, val)
end

function Audio.comp_on()
   _norns.comp_on()
end

function Audio.comp_off()
   _norns.comp_off()
end

function Audio.comp_mix(val)
   _norns.comp_mix(val)
end

function Audio.comp_param(name, val)
   _norns.comp_param(name, val)
end



-- TAPE

Audio.tape_play_open = function(file)
  _norns.tape_play_open(file)
end

Audio.tape_play_start = function()
  _norns.tape_play_start()
end

Audio.tape_play_stop = function()
  _norns.tape_play_stop()
end

Audio.tape_record_open = function(file)
  _norns.tape_record_open(file)
end

Audio.tape_record_start = function()
  _norns.tape_record_start()
end

Audio.tape_record_stop = function()
  _norns.tape_record_stop()
end



-- CUT

Audio.level_adc_cut = function(value)
  _norns.level_adc_cut(value)
end

Audio.level_ext_cut = function(value)
  _norns.level_ext_cut(value)
end

Audio.level_cut_rev = function(value)
  _norns.level_cut_rev(value)
end

Audio.level_cut = function(voice, value)
  _norns.level_cut(voice, value)
end

Audio.pan_cut = function(voice, value)
  _norns.pan_cut(voice, value)
end

Audio.level_input_cut = function(ch, voice, value)
  _norns.level_input_cut(ch, voice, value)
end

Audio.level_cut_cut = function(src, dst, value)
  _norns.level_cut_cut(src, dst, value)
end

Audio.cut_enable = function(voice, value)
  _norns.cut_enable(voice, value)
end

Audio.cut_buffer_clear_region = function(start, stop)
  _norns.cut_buffer_clear_region(start, stop)
end

Audio.cut_buffer_clear = function()
  _norns.cut_buffer_clear()
end

Audio.cut_buffer_read = function(file, start_src, start_dst, dur, ch)
  _norns.cut_buffer_read(file, start_src, start_dst, dur, ch)
end

--- params

Audio.cut_rate = function(i,v) _norns.cut_param("rate",i,v) end
Audio.cut_loop_start = function(i,v) _norns.cut_param("loop_start",i,v) end
Audio.cut_loop_end = function(i,v) _norns.cut_param("loop_end",i,v) end
Audio.cut_loop_flag = function(i,v) _norns.cut_param("loop_flag",i,v) end
Audio.cut_fade_time = function(i,v) _norns.cut_param("fade_time",i,v) end
Audio.cut_rec_level = function(i,v) _norns.cut_param("rec_level",i,v) end
Audio.cut_pre_level = function(i,v) _norns.cut_param("pre_level",i,v) end
Audio.cut_rec_flag = function(i,v) _norns.cut_param("rec_flag",i,v) end
Audio.cut_rec_offset = function(i,v) _norns.cut_param("rec_offset",i,v) end
Audio.cut_position = function(i,v) _norns.cut_param("position",i,v) end

Audio.cut_filter_fc = function(i,v) _norns.cut_param("filter_fc",i,v) end
Audio.cut_filter_fc_mod = function(i,v) _norns.cut_param("filter_fc_mod",i,v) end
Audio.cut_filter_rq = function(i,v) _norns.cut_param("filter_rq",i,v) end
Audio.cut_filter_lp = function(i,v) _norns.cut_param("filter_lp",i,v) end
Audio.cut_filter_hp = function(i,v) _norns.cut_param("filter_hp",i,v) end
Audio.cut_filter_bp = function(i,v) _norns.cut_param("filter_bp",i,v) end
Audio.cut_filter_br = function(i,v) _norns.cut_param("filter_br",i,v) end
Audio.cut_filter_dry = function(i,v) _norns.cut_param("filter_dry",i,v) end

Audio.cut_level_slew_time = function(i,v) _norns.cut_param("level_slew_time",i,v) end
Audio.cut_rate_slew_time = function(i,v) _norns.cut_param("rate_slew_time",i,v) end

Audio.cut_phase_quant = function(i,v) _norns.cut_param("phase_quant",i,v) end
Audio.poll_start_cut_phase = function() _norns.poll_start_cut_phase() end
Audio.poll_stop_cut_phase = function() _norns.poll_stop_cut_phase() end


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
