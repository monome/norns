-- add your startup code here!
-- see sys/norns.lua 


-- some recommended (?), convenient shortcuts
n = norns
e = n.engine


-- test the get_time() function
timer = function(idx, stage)
   if(idx == 1) then
      sec, usec = get_time()
      print("t: " .. sec .. "s + " .. usec .. "us")      
   end
end

timer_start(1, 0.5)



--- test some things
-- require('test_grid_timer')
-- require('test_input_devices')
