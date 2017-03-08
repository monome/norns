
version.test128 = "0.0.1"

local displaystage = 1;

grid.press = function(x, y)
   local seconds = y / 16;
   local stage = x;
   print("\n----\n start timer, period " .. seconds .. " stage " .. stage)
   timer_start(1, seconds, -1, stage) 
end


grid.lift = function(x, y)
   -- noop
end

timer = function(idx, stage)
   if(idx ==1) then
	  -- turn off the old stage
	  grid_set_led(displaystage, 1, 0)
	  -- turn on the new stage
	  displaystage = stage
	  while displaystage > 16 do
		 displaystage = displaystage - 16
	  end
	  grid_set_led(displaystage, 1, 1)
	  print("timer callback, stage " .. stage .. ", display stage " .. displaystage)
   end
end
