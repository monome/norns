--- Softcut class
-- @module softcut
-- @alias softcut

local Softcut = {}

Softcut.level = function(voice, value)
  _norns.level_cut(voice, value)
end

Softcut.pan = function(voice, value)
  _norns.pan_cut(voice, value)
end

Softcut.level_input = function(ch, voice, value)
  _norns.level_input_cut(ch, voice, value)
end

Softcut.level_voice = function(src, dst, value)
  _norns.level_cut_cut(src, dst, value)
end

Softcut.enable = function(voice, value)
  _norns.cut_enable(voice, value)
end

Softcut.buffer_clear_region = function(start, stop)
  _norns.cut_buffer_clear_region(start, stop)
end

Softcut.buffer_clear = function()
  _norns.cut_buffer_clear()
end

Softcut.buffer_read = function(file, start_src, start_dst, dur, ch)
  _norns.cut_buffer_read(file, start_src, start_dst, dur, ch)
end

--- params

Softcut.rate = function(i,v) _norns.cut_param("rate",i,v) end
Softcut.loop_start = function(i,v) _norns.cut_param("loop_start",i,v) end
Softcut.loop_end = function(i,v) _norns.cut_param("loop_end",i,v) end
Softcut.loop_flag = function(i,v) _norns.cut_param("loop_flag",i,v) end
Softcut.fade_time = function(i,v) _norns.cut_param("fade_time",i,v) end
Softcut.rec_level = function(i,v) _norns.cut_param("rec_level",i,v) end
Softcut.pre_level = function(i,v) _norns.cut_param("pre_level",i,v) end
Softcut.rec_flag = function(i,v) _norns.cut_param("rec_flag",i,v) end
Softcut.rec_offset = function(i,v) _norns.cut_param("rec_offset",i,v) end
Softcut.position = function(i,v) _norns.cut_param("position",i,v) end

Softcut.filter_fc = function(i,v) _norns.cut_param("filter_fc",i,v) end
Softcut.filter_fc_mod = function(i,v) _norns.cut_param("filter_fc_mod",i,v) end
Softcut.filter_rq = function(i,v) _norns.cut_param("filter_rq",i,v) end
Softcut.filter_lp = function(i,v) _norns.cut_param("filter_lp",i,v) end
Softcut.filter_hp = function(i,v) _norns.cut_param("filter_hp",i,v) end
Softcut.filter_bp = function(i,v) _norns.cut_param("filter_bp",i,v) end
Softcut.filter_br = function(i,v) _norns.cut_param("filter_br",i,v) end
Softcut.filter_dry = function(i,v) _norns.cut_param("filter_dry",i,v) end

Softcut.level_slew_time = function(i,v) _norns.cut_param("level_slew_time",i,v) end
Softcut.rate_slew_time = function(i,v) _norns.cut_param("rate_slew_time",i,v) end

Softcut.phase_quant = function(i,v) _norns.cut_param("phase_quant",i,v) end
Softcut.poll_start_phase = function() _norns.poll_start_cut_phase() end
Softcut.poll_stop_phase = function() _norns.poll_stop_cut_phase() end


return Softcut
