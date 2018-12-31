local SC = {}

SC.level = function(voice, value)
  _norns.level_cut(voice, value)
end

SC.pan = function(voice, value)
  _norns.pan_cut(voice, value)
end

SC.level_input_cut = function(ch, voice, value)
  _norns.level_input_cut(ch, voice, value)
end

SC.level_cut_cut = function(src, dst, value)
  _norns.level_cut_cut(src, dst, value)
end


SC.rate = function(i,v) _norns.cut_param("rate",i,v) end
SC.loop_start = function(i,v) _norns.cut_param("loop_start",i,v) end
SC.loop_end = function(i,v) _norns.cut_param("loop_end",i,v) end
SC.loop_flag = function(i,v) _norns.cut_param("loop_flag",i,v) end
SC.fade_time = function(i,v) _norns.cut_param("fade_time",i,v) end
SC.rec_level = function(i,v) _norns.cut_param("rec_level",i,v) end
SC.pre_level = function(i,v) _norns.cut_param("pre_level",i,v) end
SC.rec_flag = function(i,v) _norns.cut_param("rec_flag",i,v) end
SC.rec_offset = function(i,v) _norns.cut_param("rec_offset",i,v) end
SC.position = function(i,v) _norns.cut_param("position",i,v) end

SC.filter_fc = function(i,v) _norns.cut_param("filter_fc",i,v) end
SC.filter_fc_mod = function(i,v) _norns.cut_param("filter_fc_mod",i,v) end
SC.filter_rq = function(i,v) _norns.cut_param("filter_rq",i,v) end
SC.filter_lp = function(i,v) _norns.cut_param("filter_lp",i,v) end
SC.filter_hp = function(i,v) _norns.cut_param("filter_hp",i,v) end
SC.filter_bp = function(i,v) _norns.cut_param("filter_bp",i,v) end
SC.filter_br = function(i,v) _norns.cut_param("filter_br",i,v) end
SC.filter_dry = function(i,v) _norns.cut_param("filter_dry",i,v) end

SC.level_slew_time = function(i,v) _norns.cut_param("level_slew_time",i,v) end
SC.rate_slew_time = function(i,v) _norns.cut_param("rate_slew_time",i,v) end

SC.phase_quant = function(i,v) _norns.cut_param("phase_quant",i,v) end
SC.poll_start_phase = function() _norns.poll_start_cut_phase() end
SC.poll_stop_phase = function() _norns.poll_stop_cut_phase() end


SC.enable = function(voice, value) _norns.cut_enable(voice, value) end

SC.buffer_clear_region = function(start, stop)
   _norns.cut_buffer_clear_region(start, stop)
end

SC.buffer_clear = function() _norns.cut_buffer_clear() end

SC.buffer_read = function(file, start_src, start_dst, dur, ch)
_norns.cut_buffer_read(file, start_src, start_dst, dur, ch)
end

-- parameter factory
function SC.makeParams()
   return {
   }
end


return SC
