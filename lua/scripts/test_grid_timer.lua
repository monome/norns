require 'norns'
local grid = require 'grid'

-- current grid device
-- don't make this local, so that we can mess with it from the REPL
local g = nil

-- fn to grab grid device when we find one
setGrid = function (grid) 

-- local m = nil
grid.key = function(dev, x, y, val)
   -- use this device when we refresh
   m = dev	  
   if val == 0 then -- act on key lift
      local seconds = y / 16;
      local stage = x;
      timer_start(1, seconds, -1, stage)
   end
end

grid.add = function(dev)
   m = dev
end

grid.remove = function(dev)
   m = nil
end


local displaystage = 1;
local bright = 1;

-- timer callback (shared by all timers)
timer = function(idx, stage)
   if idx == 1 then -- check that this CB was fired from timer 1
      if m ~= nil then
	 -- turn off the old stage
	 m:led(displaystage, 1, 0)
	 -- turn on the new stage
	 displaystage = stage
	 while displaystage > 16 do
	    displaystage = displaystage - 16
	 end
	 m:led(displaystage, 1, bright)
	 m:refresh()
	 bright = bright + 1
	 while (bright > 15) do bright = bright - 15 end		 
      end
   end
end


