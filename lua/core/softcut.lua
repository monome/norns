--- Softcut module
--
-- API for controlling the "softcut" buffer processor
-- includes low-level setters and mid-level utilities
--
-- IMPORTANT: all indices are 1-based, per lua convention!
-- this applies to indices for selecting voice, ADC/DAC channel, buffer, &c.
-- however, _quantities_ (including units of time) can still start at zero.
--
-- @module softcut
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

--- set output level of each voice
-- @tparam int voice : voice index
-- @tparam int amp : linear amplitude
SC.level = function(voice, amp)
  _norns.level_cut(voice, amp)
end

--- set pan position of each voice. 
-- -1 == full left, +1 == full right, 0 == centered
-- @tparam int voice : voice index
-- @tparam number pos : position in [-1, 1]
SC.pan = function(voice, pos)
  _norns.pan_cut(voice, pos)
end

--- set input level to each voice/channel. 
-- @tparam int ch : ADC channel index
-- @tparam int voice : voice index 
-- @tparam number pos : position in [-1, 1]
SC.level_input_cut = function(ch, voice, pos)
  _norns.level_input_cut(ch, voice, pos)
end

--- set mix matrix, voice output to voice input. 
-- @tparam number src : source voice index
-- @tparam number dst : destination voice index
-- @tparam number amp : linear amplitude
SC.level_cut_cut = function(src, dst, amp)
  _norns.level_cut_cut(src, dst, amp) 
end

--- set play status. 
-- @tparam int voice : voice index
-- @tparam int state : off/on (0,1)
SC.play = function(voice,state) _norns.cut_param("play_flag",voice,state) end

--- set playback rate. 
-- @tparam int voice : voice index
-- @tparam number rate : speed of read/write head (unitless; 1 == normal)
SC.rate = function(voice,rate) _norns.cut_param("rate",voice,rate) end

--- set loop start.
-- @tparam int voice : voice index
-- @tparam number pos : loop start position in seconds
SC.loop_start = function(voice,pos) _norns.cut_param("loop_start",voice,pos) end

--- set loop end. 
-- @tparam int voice : voice index
-- @tparam number pos : loop end position in seconds
SC.loop_end = function(voice,pos) _norns.cut_param("loop_end",voice,pos) end

--- set loop mode. 
-- "0" indicates one-shot mode: voice will play to the loop endpoint, fadeout and stop.
-- "1" indicates crossfaded looping mode.
-- @tparam int voice : voice index
-- @tparam int state : off/on (0,1)
SC.loop = function(voice,state) _norns.cut_param("loop_flag",voice,state) end

--- set fade time. 
-- @tparam int voice : voice index
-- @tparam number pos : loop start position in seconds
SC.fade_time = function(voice,pos) _norns.cut_param("fade_time",voice,pos) end

--- set record level.
-- this sets the realtime-modulated record level,
-- by which incoming signal is scaled before writing to the buffer
-- `recpre` slew level applies
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude of new signal
SC.rec_level = function(voice,amp) _norns.cut_param("rec_level",voice,amp) end

--- set pre level (overdub preserve.)
-- this sets the realtime-modulated "preserve" level,
-- by which existing material is scaled on each pass of the write head.
-- `recpre` slew level applies
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude of preserved signal 
SC.pre_level = function(voice,amp) _norns.cut_param("pre_level",voice,amp) end

--- set record state. 
-- @tparam int voice : voice number (1-?)
-- @tparam int state : off/on (0,1)
SC.rec = function(voice,state) _norns.cut_param("rec_flag",voice,state) end
--- set record head offset
SC.rec_offset = function(voice,value) _norns.cut_param("rec_offset",voice,value) end
--- set play position
SC.position = function(voice,value) _norns.cut_param("position",voice,value) end

--- specify buffer used by voice. 
-- @tparam int i : voice number
-- @tparam int b : buffer number (1,2)
SC.buffer = function(i,b) _norns.cut_param_ii("buffer",i,b) end

--- synchronize two voices. 
--- position of "dst" will be immediately set to that of "source"
-- @tparam int src : source voice index
-- @tparam int dst : destination voice index
-- @tparam number offset : additional offset in seconds
SC.voice_sync = function(src, dst, offset) _norns.cut_param_iif("voice_sync",src,dest,offset) end

--- set pre_filter cutoff frequency. 
--- @tparam int voice : voice index
--- @tparam number fc : cutoff frequency in Hz
SC.pre_filter_fc = function(voice,fc) _norns.cut_param("pre_filter_fc",voice,fc) end

--- set pre_filter amount of rate modulation. 
-- this parameter controls the amount by which the current rate affects filter cutoff frequency 
-- (always in a negative direction, towards zero.) 
-- with mod == 1, setting rate = 0 will also fully close the filter. 
-- this can be useful as a crude anti-aliasing method...
--- @tparam int voice : voice index
--- @tparam number amount : modulation amount in [0, 1]
SC.pre_filter_fc_mod = function(voice,amount) _norns.cut_param("pre_filter_fc_mod",voice,amount) end

--- set pre_filter reciprocal of Q-factor. 
-- the reciprocal of the filter's Q-factor is a measure of bandwidth,
-- that is independent of center frequency.
-- RQ ~= 0 will result in self-oscillation;
-- RQ == 4 gives a bandwidth of 2 octaves.
-- @tparam int voice : voice index
-- @tparam number rq : reciprocal of filter Q-factor for voice
SC.pre_filter_rq = function(voice,rq) _norns.cut_param("pre_filter_rq",voice,rq) end

--- set pre_filter lowpass output level. 
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude 
SC.pre_filter_lp = function(voice,amp) _norns.cut_param("pre_filter_lp",voice,amp) end

--- set pre-filter highpass output level. 
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude 
SC.pre_filter_hp = function(voice,amp) _norns.cut_param("pre_filter_hp",voice,amp) end

--- set pre-filter bandpass output level.
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude 
SC.pre_filter_bp = function(voice,amp) _norns.cut_param("pre_filter_bp",voice,amp) end

--- set pre_filter band-reject output level.
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude 
SC.pre_filter_br = function(voice,amp) _norns.cut_param("pre_filter_br",voice,amp) end

--- set pre_filter dry output level. 
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude 
SC.pre_filter_dry = function(voice,amp) _norns.cut_param("pre_filter_dry",voice,amp) end


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

--- set post-filter cutoff
-- @tparam int voice : voice index
-- @tparam number value : cutoff frequency in Hz
SC.post_filter_fc = function(voice,value) _norns.cut_param("post_filter_fc",voice,value) end

--- set post-filter reciprocal of Q
-- the reciprocal of the filter's Q factor is a measure of bandwidth,
-- that is independent of center frequency. 
-- RQ ~= 0 will result in self oscillation;
-- RQ == 4 gives a bandwidth of 2 octaves. 
-- @tparam int voice : voice index
-- @tparam number rq : reciprocal of filter Q-factor for voice
SC.post_filter_rq = function(voice,rq) _norns.cut_param("post_filter_rq",voice,rq) end


--- set post_filter lowpass output level. 
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude 
SC.post_filter_lp = function(voice,amp) _norns.cut_param("post_filter_lp",voice,amp) end

--- set post-filter highpass output level. 
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude 
SC.post_filter_hp = function(voice,amp) _norns.cut_param("post_filter_hp",voice,amp) end

--- set post-filter bandpass output level.
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude 
SC.post_filter_bp = function(voice,amp) _norns.cut_param("post_filter_bp",voice,amp) end

--- set post_filter band-reject output level.
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude 
SC.post_filter_br = function(voice,amp) _norns.cut_param("post_filter_br",voice,amp) end

--- set post_filter dry output level. 
-- @tparam int voice : voice index
-- @tparam number amp : linear amplitude 
SC.post_filter_dry = function(voice,amp) _norns.cut_param("post_filter_dry",voice,amp) end

--- set level-slew time
-- this slew time applies to level at all mix points: ADC->voice, voice->voice, &c.
-- @tparam int voice : voice index
-- @tparam number time : exponential slew time in seconds (-60db convergence)
SC.level_slew_time = function(voice,time) _norns.cut_param("level_slew_time",voice,time) end

--- set pan slew time
-- @tparam int voice : voice index
-- @tparam number time : exponential slew time in seconds (-60db convergence)
SC.pan_slew_time = function(voice,time) _norns.cut_param("pan_slew_time",voice,time) end

--- set recpre slew tiem
-- affects slew time for record and pre levels
-- @tparam int voice : voice index
-- @tparam number time : exponential slew time in seconds (-60db convergence)
SC.recpre_slew_time = function(voice,time) _norns.cut_param("recpre_slew_time",voice,time) end

--- set rate slew time
-- affects slew time for rate
-- @tparam int voice : voice index
-- @tparam number time : exponential slew time in seconds (-60db convergence)
SC.rate_slew_time = function(voice,time) _norns.cut_param("rate_slew_time",voice,time) end

--- set phase poll quantum
-- e.g. 0.25 will produce 4 updates per second with rate=1
-- judicious use of this parameter is preferable to using a very fast poll (for performance,)
-- or polling at arbitrary rate (for accuracy when rate is slewed.)
-- @tparam int voice : voice index
-- @tparam number quantum : phase reporting interval, in seconds
SC.phase_quant = function(voice,quantum) _norns.cut_param("phase_quant",voice,quantum) end

--- set phase poll offset in frames
-- @tparam int voice : voice index
-- @tparam number offset : phase poll offset in seconds
SC.phase_offset = function(voice,offset) _norns.cut_param("phase_offset",voice,offset) end

--- start phase poll
SC.poll_start_phase = function() _norns.poll_start_cut_phase() end

--- stop phase poll
SC.poll_stop_phase = function() _norns.poll_stop_cut_phase() end

--- set voice enable
-- disabled voices have no effect and consume basically zero CPU
-- @tparam int voice : voice number (1-?)
-- @tparam int state : off/on (0,1)
SC.enable = function(voice, state) _norns.cut_enable(voice, state) end

----------------
--- TODO: complete function doc comments below here!

--- clear all buffers completely
SC.buffer_clear = function() _norns.cut_buffer_clear() end

--- clear one buffer completely
-- @tparam int channel : buffer channel index (1-based)
SC.buffer_clear_channel = function(channel) _norns.cut_buffer_clear_channel(channel) end

--- clear region (both channels)
-- @tparam number start : start point in seconds
-- @tparam number dur : duration in seconds
SC.buffer_clear_region = function(start, dur)
  _norns.cut_buffer_clear_region(start, dur)
end

--- clear region of single channel
-- @tparam int ch : buffer channel index (1-based)
-- @tparam number start : start point in seconds
-- @tparam number dur : duration in seconds
SC.buffer_clear_region_channel = function(ch, start, dur)
  _norns.cut_buffer_clear_region_channel(ch, start, dur)
end

--- read mono soundfile to arbitrary region of single buffer
-- @tparam string file : input file path
-- @tparam number start_src : start point in source, in seconds
-- @tparam number start_dst : start point in destination, in seconds
-- @tparam number dur : duration in seconds. if -1, read as much as possible.
-- @tparam int ch_src : soundfie channel to read
-- @tparam int ch_dst : buffer channel to write
SC.buffer_read_mono = function(file, start_src, start_dst, dur, ch_src, ch_dst)
  _norns.cut_buffer_read_mono(file, start_src, start_dst, dur, ch_src, ch_dst)
end

--- read stereo soundfile to an arbitrary region in both buffers
-- @tparam string file : input file path
-- @tparam number start_src : start point in source, in seconds
-- @tparam number start_dst : start point in destination, in seconds
-- @tparam number dur : duration in seconds. if -1, read as much as possible
SC.buffer_read_stereo = function(file, start_src, start_dst, dur)
  _norns.cut_buffer_read_stereo(file, start_src, start_dst, dur)
end

--- write an arbitrary buffer region to soundfile (mono)
-- @tparam string file : output file path
-- @tparam number start : start point in seconds
-- @tparam number dur : duration in seconds. if -1, read as much as possible
-- @tparam int ch : buffer channel index (1-based)
SC.buffer_write_mono = function(file, start, dur, ch)
  _norns.cut_buffer_write_mono(file, start, dur, ch)
end

--- write an arbitrary region from both buffers to stereo soundfile
-- @tparam string file : output file path
-- @tparam number start : start point in seconds
-- @tparam number dur : duration in seconds. if -1, read as much as possible
SC.buffer_write_stereo = function(file, start, dur)
  _norns.cut_buffer_write_stereo(file, start, dur)
end

--- set function for phase poll
-- @tparam function func : callback function. this function should take two parameters  (voice, phase)
SC.event_phase = function(func) _norns.softcut_phase = func end


-------------------------------
-- @section utilities

--- reset state of softcut process on backend.
-- this should correspond to the values returned by the `defaults()` function above.
function SC.reset()
   _norns.cut_reset()
  SC.event_phase(norns.none)
end

--- get the default state of the softcut system
-- this returns a table for each voice,
-- in which each key corresponds to the name of one of the setter functions defined above.
-- for parameters with one value per voice, the corresponding entry is also a single value.
-- for parameters with multiple values (e.g. matrix indices), the entry is a table.
-- NB: these values are synchronized by hand with those specified in the softcut cpp sources 
-- @treturn table table of parameter states for each voice
function SC.defaults()
   zeros = {}
   
   for i=1,SC.VOICE_COUNT do
      zeros[i] = 0
   end
   
   local state = {}
   for i=1,SC.VOICE_COUNT do
      state[i] = {}
      
     state[i].enable = 0
     state[i].play = 0
     state[i].record = 0
     
     state[i].buffer = (i%2 + 1)
     state[i].level =0
     state[i].pan = 0
     
     state[i].level_input_cut = {0,0}
     state[i].level_cut_cut = zeros
     
     state[i].rate = 1
     state[i].loop_start = (i-1)*2
     state[i].loop_end = (i-1)*2+1
     state[i].loop = 1
     
     state[i].fade_time =  0.0005
     state[i].rec_level = 0
     state[i].pre_level = 0
     state[i].rec = 0
     state[i].rec_offset = -0.00015
     state[i].position = 0
     
     state[i].pre_filter_fc = 16000
     state[i].pre_filter_dry = 0
     state[i].pre_filter_lp = 1
     state[i].pre_filter_hp = 0
     state[i].pre_filter_bp = 0
     state[i].pre_filter_br = 0
     state[i].pre_filter_fc_mod = 1

     state[i].post_filter_fc = 12000
     state[i].post_filter_dry = 0
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

return SC
