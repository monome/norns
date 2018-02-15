-- test audio looping with grid and softcut

engine = 'SoftCut'

local audio = require 'audio'
audio.monitor_on()
audio.monitor_mono()
audio.monitor_level(1.0)

local pos1 = 0;

local poll_phase1 = poll.set('phase_1', function(sec)
      pos1 = sec
      redraw()
end)

init = function()
    print("grid/loop")
    e.listCommands()
    poll.listNames()
    e.start(1, 1)
    -- e.end(1, 9)
   e.rec(1, 1.0)
   e.pre(1, 0.7)
   e.fade(1, 0.25)
   e.start(1)
end

gridkey = function(x, y, state)
-- TODO
   g:refresh()
end

gridredraw = function()
  g:all(1)
  -- TODO
end

enc = function(n,delta)
    -- TODO
    redraw() 
end

key = function(n)
-- TODO
end

redraw = function()
    s.clear()
    if g ~= nil then
       gridredraw()
    end
end 

cleanup = function()
   if poll_phase1 then p:stop() end
end
