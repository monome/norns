
-- test the get_time() function
timer = function(idx, stage)
   if(idx == 1) then
      sec, usec = get_time()
      print("t: " .. sec .. "s + " .. usec .. "us")      
   end
end

timer_start(1, 0.5)

