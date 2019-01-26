--- Softcut module
--
-- API for controlling the "softcut" buffer processor
-- includes low-level setters and mid-level utilities
--
-- @module softcut
-- @alias softcut

local SC = {}

local controlspec = require 'core/controlspec'

-------------------------------
-- @section constants

-- @field number of voices
SC.VOICE_COUNT = 6
-- @field length of buffer in seconds
SC.BUFFER_SIZE = 16777216 / 48000

-------------------------------
-- @section setters
-- NB: voice indices are zero-based!

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

SC.buffer_read = function(filesleep, start_src, start_dst, dur, ch)
  _norns.cut_buffer_read(file, start_src, start_dst, dur, ch)
end


------------------------------
-- @section utilities

-- controlspec factory
-- @return an array of tables, one per voice.
-- each table contains an entry for each softcut parameter.
-- each entry is a parameter argument list configured for that voice+param
function SC.params()
  -- @fixme should memoize
  local specs = {}
  local voice=0
  while voice < SC.VOICE_COUNT do
    local spec = {
      -- voice enable
      enable = { type="number", min=0, max=1, default=0, formatter="" },
      -- levels
      -- @fixme: use dB / taper?
      level = { type="control", controlspec=controlspec.new(0, 0, 'lin', 0, 0.25, "") },	 
      pan = { type="control", controlspec=controlspec.new(-1, 1, 'lin', 0, 0, "") },
      level_input_cut = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0.5, "") },
      level_cut_cut = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      -- timing
      rate = { type="control", controlspec=controlspec.new(-8, 8, 'lin', 0, 0, "") },
      loop_start = { type="control", controlspec=controlspec.new(0, SC.BUFFER_SIZE, 'lin', 0, voice*2.5, "sec") },
      loop_end = { type="control", controlspec=controlspec.new(0, SC.BUFFER_SIZE, 'lin', 0, voice*2.5 + 2, "sec") },
      loop_flag = { type="number", min=1, max=1, default=1, formatter=""},
      fade_time = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      -- recording parameters
      rec_level = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      pre_level = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      rec_flag = { type="number", min=1, max=1, default=1, formatter=""},
      rec_offset = { type="number", min=-100, max=100, default=-8, formatter="samples"},
      -- jump to position
      position = { type="control", controlspec=controlspec.new(0, SC.BUFFER_SIZE, 'lin', 0, voice*2.5, "sec") },
      -- filter
      filter_fc = { type="control", controlspec=controlspec.new(10, 12000, 'exp', 1, 12000, "Hz") },
      filter_fc_mod = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 1, "") },
      filter_rq = { type="control", controlspec=controlspec.new(0.0005, 8.0, 'exp', 0, 2.0, "") },
      -- @fixme use dB / taper?
      filter_lp = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 1, "") },
      filter_hp = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      filter_bp = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      filter_br = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      filter_dry = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      -- slew times
      level_slew_time = { type="control", controlspec=controlspec.new(0, 8, 'lin', 0, 0, "") },
      rate_slew_time = { type="control", controlspec=controlspec.new(0, 8, 'lin', 0, 0, "") },
      -- poll quantization unit
      phase_quant = { type="control", controlspec=controlspec.new(0, 8, 'lin', 0, 0.125, "") },
    }
    -- assign name, id and action 
    for k,v in pairs(spec) do 
      local z = voice
      spec[k].id = k
      spec[k].name = "cut"..(z+1)..k
      local act = SC[k]
      if act == nil then
        print("warning: didn't find SoftCut voice method: "..k)
      end
      spec[k].action = function(x) act(z, x) end
    end
    specs[voice+1] = spec
    voice = voice + 1
  end

  return specs
end

--[[ FIXME:
-- control factory
-- @return array of controls, using .specs() and connected to setters
function SC.controls
   local ctl = {}
   local specs = CS.specs()
   -- @fixme should definitely memoize
   for voice,spec in ipairs(specs) do
      local c = {}
      for k,spec in specs[v] do
	 c = control.new(k, k, spec, "")
	 local act = SC[k]
	 if act == nil then
	    print("warning: didn't find SoftCut voice method: "..k)
	 end
	 -- @fixme? note the base-0 voice indexing
	 c.action = function(value) act(v-1, value) end
      end
      v = v + 1
   end
end
--]]

return SC
