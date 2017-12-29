t = metro[2]
t.time = 1
t.count = -1

-- test the get_time() function
t.callback = function(stage)
   print("> "..stage)
   --sec, usec = get_time()
   --   print("stage: ".. stage .. " t: " .. sec .. "s + " .. usec .. "us")
   if stage == 1 then print ('...')
   elseif stage == n then print('!!!') end
end

t:start()
