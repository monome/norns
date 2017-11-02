local timer = require 'timer'

local t = timer[1]

-- test the get_time() function
t.callback = function(stage)
   sec, usec = get_time()
   print("t: " .. sec .. "s + " .. usec .. "us")      
end

t:start(1.0)

norns.cleanup = function()
   t:stop()
end
