-- test audio looping with grid and softcut

engine = 'SoftCut'

local audio = require 'audio'
local poll = require 'poll'
local math = require 'math'

audio.monitor_on()
audio.monitor_mono()
audio.monitor_level(0.5)

-- current phase of loops, as grid column numbers
local grid_phase = { 1, 1 }

-- max logical loop endpoint
local loop_max = 4.0

-- helper to update displayed phase on grid
-- @param pos: current position in seconds
local update_grid_phase = function(channel, pos)
    local x = math.floor(pos / loop_max * 16.0)
    grid_phase[channel] = x
    -- gridredraw()
    redraw()
end

-- phase polls
local p1 = nil
local p2 = nil


-- local parameter values
local rate = 1.0
local pre = 0.5

-- increment functions
function inc_rate(delta)
    local val = rate + (delta * 0.125)
    if val > 4.0 then val = 4.0 end
    if val < -4.0 then val = -4.0 end
    rate = val
    e.rate(1, val)
    e.rate(2, val)
end

function inc_pre(delta)
    local val = pre + (delta * 0.05)
    if val > 0.9 then val = 0.9 end
    if val < 0.0 then val = 0.0 end
    pre = val
    e.pre(1, val)
    e.pre(2, val)
end

-- the main init function
init = function()
    print("grid/loop")
    -- connect i/o patch points
    
    e.adc_rec(1, 1, 1.0)
    e.adc_rec(1, 2, 1.0)
	
    e.play_dac(1, 1, 1.0)
    e.play_dac(2, 2, 1.0)

    -- first two channels in a silly delay mode

    -- start and end define the "delay time"
    e.loopStart(1, 0.5)
    e.loopStart(2, 0.5)
    e.loopEnd(1, 2.0)
    e.loopEnd(2, 2.5)

    -- turn on relevant params and start running
    for i=1,2 do
    	e.amp(i, 0.5)
	e.rec(i, 1.0)
	e.pre(i, pre) -- pre-level is like "feedback"
	e.rate(i, rate)
	e.recRun(i, 1) -- record flag is on (otherwise buffer contents unchanged)
	e.start(i) -- begin running
    end

    -- setup phase poll for feedback
    p1 = poll.set('phase_1', function(val) update_grid_phase(1, val) end)
    p1.time = 0.05
    p1:start()
    
    p2 = poll.set('phase_2', function(val) update_grid_phase(2, val) end)
    p2.time = 0.05
    p2:start()
end

gridkey = function(x, y, state)
   g:refresh()
end

--[[ untested
gridredraw = function()
  g:all(0)
  g:led(1, grid_phase[1])
  g:led(2, grid_phase[2])
  g:refresh()
end
--]]

redraw = function()

end

enc = function(n,delta)
    if n==2 then
        inc_rate(delta)
    elseif n==3 then
        inc_pre(delta)
    end
    redraw() 
end


key = function(n)
    if n==2 then
        e.reset(1)
    elseif n==3 then
        e.reset(2)
    end
end

redraw = function()
    s.clear()
    s.line_width(4)
    s.level(10)
    local x
    local y
    for i=1,2 do
        x = grid_phase[i] * 4
	y = i * 8 + 40
    	s.move(x, y)
	s.line(x + 4, y)
	s.stroke()
    end

    s.move(0,10)    
    s.text('rate = ' .. rate)
    s.move(0,20)    
    s.text('pre = ' .. pre)	
end 



cleanup = function()
   if p then p:stop() end
end
