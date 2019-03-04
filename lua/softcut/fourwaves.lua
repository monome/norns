-- four voice wave player

local stop
local t = {metro.init(),metro.init(),metro.init(),metro.init()}
local d = 1

local m = require 'musicutil'

local voices = {4,4,4,4}

local function new_voice()
  local x = 0
  local n = 1
  for i=1,4 do
    if x < voices[i] then
      x = voices[i]
      n = i
    end 
    voices[i] = voices[i]+1
  end
  voices[n] = 0;
  return n;
end


local sc = {}

function sc.init(file)
  file = file or "audio/common/waves/01.wav"
  local ch, dur, sr = audio.file_info(file)
  d = dur/sr

  print("fourwaves: loading "..file)
  softcut.buffer_read_mono(dust_dir..file,0,1,d,0,0)
  print("fourwaves: dur "..d)

  audio.level_cut(1.0)

  for i=1,4 do
    softcut.level(i,1.0)
    softcut.pan(i, 0.5)
    softcut.buffer(i, 1)
    softcut.play(i, 1)
    softcut.rec(i, 0)
    softcut.rate(i, 0)
    softcut.rate_slew_time(i,0)
    softcut.loop(i, 0)
    softcut.loop_start(i,1)
    softcut.loop_end(i,dur)
    softcut.fade_time(i, 0.01)
    softcut.position(i, 1)
    softcut.enable(i, 1)
    softcut.filter_dry(i, 0.125);
    softcut.filter_fc(i, 1200);
    softcut.filter_lp(i, 0);
    softcut.filter_bp(i, 1.0);
    softcut.filter_rq(i, 2.0);
  end

  --params:add_separator()
  --local p = softcut.params()
  --params:add(p[1].rate)
end

function stop(v)
  t[v]:stop()
  softcut.rate(v,0)
end

function sc.note(n)
  local r = m.interval_to_ratio(n)
  local v = new_voice()
  t[v]:stop()
  t[v].time = d*r
  t[v].event = function() stop(v) end
  t[v]:start()
  softcut.rate(v,r)
  softcut.position(v,1)
end

return sc
