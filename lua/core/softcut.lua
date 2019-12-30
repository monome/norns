--- Softcut module
--
-- API for controlling the "softcut" buffer processor
-- includes low-level setters and mid-level utilities
--
-- @classmod softcut
-- @alias SC

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

--- set level of each voice
SC.level = function(voice, value)
  _norns.level_cut(voice, value)
end

--- set pan of each voice
SC.pan = function(voice, value)
  _norns.pan_cut(voice, value)
end

--- set input level to each voice/channel
SC.level_input_cut = function(ch, voice, value)
  _norns.level_input_cut(ch, voice, value)
end

--- set mix matrix, voice output to voice input
-- @tparam number src source
-- @tparam number dst destination
-- @tparam number value value
SC.level_cut_cut = function(src, dst, value)
  _norns.level_cut_cut(src, dst, value)
end

--- set play status
-- @tparam integer voice : voice number (1-?)
-- @tparam integer state : off/on (0,1)
SC.play = function(voice,state) _norns.cut_param("play_flag",voice,state) end
-- set playback rate
SC.rate = function(voice,value) _norns.cut_param("rate",voice,value) end
--- set loop start
SC.loop_start = function(voice,value) _norns.cut_param("loop_start",voice,value) end
--- set loop end
SC.loop_end = function(voice,value) _norns.cut_param("loop_end",voice,value) end
--- set loop mode
-- @tparam integer voice : voice number (1-?)
-- @tparam integer state : off/on (0,1)
SC.loop = function(voice,state) _norns.cut_param("loop_flag",voice,state) end
--- set fade time
SC.fade_time = function(voice,value) _norns.cut_param("fade_time",voice,value) end
--- set record level
SC.rec_level = function(voice,value) _norns.cut_param("rec_level",voice,value) end
--- set pre level (overdub preserve)
SC.pre_level = function(voice,value) _norns.cut_param("pre_level",voice,value) end
--- set record status
-- @tparam integer voice : voice number (1-?)
-- @tparam integer state : off/on (0,1)
SC.rec = function(voice,state) _norns.cut_param("rec_flag",voice,state) end
--- set record head offset
SC.rec_offset = function(voice,value) _norns.cut_param("rec_offset",voice,value) end
--- set play position
SC.position = function(voice,value) _norns.cut_param("position",voice,value) end

--- specify buffer used by voice
-- @tparam integer i : voice number
-- @tparam integer b : buffer number (1,2)
SC.buffer = function(i,b) _norns.cut_param_ii("buffer",i,b) end
--- sync two voices
SC.voice_sync = function(src, dest, v) _norns.cut_param_iif("voice_sync",src,dest,v) end

--- set pre_filter cutoff
SC.pre_filter_fc = function(voice,value) _norns.cut_param("pre_filter_fc",voice,value) end
--- set pre_filter mod
SC.pre_filter_fc_mod = function(voice,value) _norns.cut_param("pre_filter_fc_mod",voice,value) end
--- set pre_filter q
SC.pre_filter_rq = function(voice,value) _norns.cut_param("pre_filter_rq",voice,value) end
--- set pre_filter lp
SC.pre_filter_lp = function(voice,value) _norns.cut_param("pre_filter_lp",voice,value) end
--- set pre_filter hp
SC.pre_filter_hp = function(voice,value) _norns.cut_param("pre_filter_hp",voice,value) end
--- set pre_filter bp
SC.pre_filter_bp = function(voice,value) _norns.cut_param("pre_filter_bp",voice,value) end
--- set pre_filter br
SC.pre_filter_br = function(voice,value) _norns.cut_param("pre_filter_br",voice,value) end
--- set pre_filter dry
SC.pre_filter_dry = function(voice,value) _norns.cut_param("pre_filter_dry",voice,value) end


---------------------
-- wrappers around pre_filter, for backwards compatibility
SC.filter_fc = function(voice,value) SC.pre_filter_fc(voice, value) end 
SC.filter_fc_mod = function(voice,value) SC.pre_filter_fc_mod(voice, value) end 
SC.filter_rq = function(voice,value) SC.pre_filter_rq(voice, value) end 
SC.filter_lp = function(voice,value) SC.pre_filter_lp(voice, value) end 
SC.filter_hp = function(voice,value) SC.pre_filter_hp(voice, value) end 
SC.filter_bp = function(voice,value) SC.pre_filter_bp(voice, value) end 
SC.filter_br = function(voice,value) SC.pre_filter_br(voice, value) end 
SC.filter_dry = function(voice,value) SC.pre_filter_dry(voice, value) end 

--- set post_filter cutoff
SC.post_filter_fc = function(voice,value) _norns.cut_param("post_filter_fc",voice,value) end
--- set post_filter mod
SC.post_filter_fc_mod = function(voice,value) _norns.cut_param("post_filter_fc_mod",voice,value) end
--- set post_filter q
SC.post_filter_rq = function(voice,value) _norns.cut_param("post_filter_rq",voice,value) end
--- set post_filter lp
SC.post_filter_lp = function(voice,value) _norns.cut_param("post_filter_lp",voice,value) end
--- set post_filter hp
SC.post_filter_hp = function(voice,value) _norns.cut_param("post_filter_hp",voice,value) end
--- set post_filter bp
SC.post_filter_bp = function(voice,value) _norns.cut_param("post_filter_bp",voice,value) end
--- set post_filter br
SC.post_filter_br = function(voice,value) _norns.cut_param("post_filter_br",voice,value) end
--- set post_filter dry
SC.post_filter_dry = function(voice,value) _norns.cut_param("post_filter_dry",voice,value) end


--- set level slew time
SC.level_slew_time = function(voice,value) _norns.cut_param("level_slew_time",voice,value) end
--- set pan slew time
SC.pan_slew_time = function(voice,value) _norns.cut_param("pan_slew_time",voice,value) end
--- set recpre slew time
SC.recpre_slew_time = function(voice,value) _norns.cut_param("recpre_slew_time",voice,value) end
--- set rate slew time
SC.rate_slew_time = function(voice,value) _norns.cut_param("rate_slew_time",voice,value) end

--- set phase poll quantization
SC.phase_quant = function(voice,value) _norns.cut_param("phase_quant",voice,value) end
--- set phase poll offset
SC.phase_offset = function(voice,value) _norns.cut_param("phase_offset",voice,value) end
--- start phase poll
SC.poll_start_phase = function() _norns.poll_start_cut_phase() end
--- stop phase poll
SC.poll_stop_phase = function() _norns.poll_stop_cut_phase() end

--- set voice enable
-- @tparam integer voice : voice number (1-?)
-- @tparam integer state : off/on (0,1)
SC.enable = function(voice, state) _norns.cut_enable(voice, state) end

--- clear all buffers
SC.buffer_clear = function() _norns.cut_buffer_clear() end
--- clear one channel of buffer
SC.buffer_clear_channel = function(i) _norns.cut_buffer_clear_channel(i) end
--- clear region (both channels)
SC.buffer_clear_region = function(start, stop)
  _norns.cut_buffer_clear_region(start, stop)
end
--- clear region of single channel
SC.buffer_clear_region_channel = function(ch, start, stop)
  _norns.cut_buffer_clear_region_channel(ch, start, stop)
end

--- read file to one channel
SC.buffer_read_mono = function(file, start_src, start_dst, dur, ch_src, ch_dst)
  _norns.cut_buffer_read_mono(file, start_src, start_dst, dur, ch_src, ch_dst)
end
--- read file, stereo
SC.buffer_read_stereo = function(file, start_src, start_dst, dur)
  _norns.cut_buffer_read_stereo(file, start_src, start_dst, dur)
end

--- write file, mono
SC.buffer_write_mono = function(file, start, dur, ch)
  _norns.cut_buffer_write_mono(file, start, dur, ch)
end
--- write file, stereo
SC.buffer_write_stereo = function(file, start, dur)
  _norns.cut_buffer_write_stereo(file, start, dur)
end

--- set function for phase poll
SC.event_phase = function(f) _norns.softcut_phase = f end


-------------------------------
-- @section utilities

--- reset softcut params
function SC.reset()
   _norns.cut_reset()
  SC.event_phase(norns.none)
end

--- get the default state of the softcut system
--- @treturn table table of parameter states for each voice
function SC.defaults()
  local state = {}
  for i=1,SC.COUNT do
     state[i] = 0;
     state[i].enable =0
     state[i].play =0
     state[i].buffer = (i%2 + 1)
     state[i].level =0
     state[i].pan =0.5
     state[i].level_input_cut =i,0
     state[i].level_input_cut =i,0
     state[i].level_cut_cut =i,0
     state[i].level_cut_cut =i,0
     state[i].rate =1
     state[i].loop_start = (i-1)*2
     state[i].loop_end = (i-1)*2+1
     state[i].loop = 1
     state[i].fade_time =  0.0005
     state[i].rec_level = 0
     state[i].pre_level = 0
     state[i].rec = 0
     state[i].rec_offset = -0.00015
     state[i].position = 0
     state[i].pre_filter_dry = 1
     state[i].pre_filter_lp = 0
     state[i].pre_filter_hp = 0
     state[i].pre_filter_bp = 0
     state[i].pre_filter_br = 0
     state[i].post_filter_dry = 1
     state[i].post_filter_lp = 0
     state[i].post_filter_hp = 0
     state[i].post_filter_bp = 0
     state[i].post_filter_br = 0
     state[i].level_slew_time = 0.001
     state[i].rate_slew_time = 0.001
     state[i].phase_quant = 1
     state[i].phase_offset = 0
  end
  return state
end


--- controlspec factory
-- each table contains an entry for each softcut parameter.
-- each entry is a parameter argument list configured for that voice+param
-- @return an array of tables, one per voice.
function SC.params()
  -- @fixme should memoize
  local specs = {}
  local voice=1
  while voice <= SC.VOICE_COUNT do
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
      loop = { type="number", min=0, max=1, default=1, formatter=""},
      fade_time = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      -- recording parameters
      rec_level = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      pre_level = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      play = { type="number", min=0, max=1, default=1, formatter=""},
      rec = { type="number", min=0, max=1, default=1, formatter=""},
      rec_offset = { type="number", min=-100, max=100, default=-8, formatter="samples"},
      -- jump to position
      position = { type="control", controlspec=controlspec.new(0, SC.BUFFER_SIZE, 'lin', 0, voice*2.5, "sec") },
      -- pre filter
      pre_filter_fc = { type="control", controlspec=controlspec.new(10, 12000, 'exp', 1, 12000, "Hz") },
      pre_filter_fc_mod = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 1, "") },
      pre_filter_rq = { type="control", controlspec=controlspec.new(0.0005, 8.0, 'exp', 0, 2.0, "") },
      -- @fixme use dB / taper?
      pre_filter_lp = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 1, "") },
      pre_filter_hp = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      pre_filter_bp = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      pre_filter_br = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      pre_filter_dry = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      -- post filter
      post_filter_fc = { type="control", controlspec=controlspec.new(10, 12000, 'exp', 1, 12000, "Hz") },
      post_filter_fc_mod = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 1, "") },
      post_filter_rq = { type="control", controlspec=controlspec.new(0.0005, 8.0, 'exp', 0, 2.0, "") },
      -- @fixme use dB / taper?
      post_filter_lp = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 1, "") },
      post_filter_hp = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      post_filter_bp = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      post_filter_br = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },
      post_filter_dry = { type="control", controlspec=controlspec.new(0, 1, 'lin', 0, 0, "") },

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
      spec[k].name = "cut"..z..k
      local act = SC[k]
      if act == nil then
        print("warning: didn't find SoftCut voice method: "..k)
      end
      spec[k].action = function(x) act(z, x) end
    end
    specs[voice] = spec
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
