local metro = require 'metro'

-- leep this global so we can ply with it from the repl
t = metro[2]
t.time = 1
t.count = -1

-- test the get_time() function
t.callback = function(stage)
	print("-")
--   sec, usec = get_time()
   --   print("stage: ".. stage .. " t: " .. sec .. "s + " .. usec .. "us")
   if stage == 1 then print ('...')
   elseif stage == n then print('!!!') end
end

t:start()

norns.script.cleanup = function()
   t:stop()
   s:stop()
end


local tick = require 'metro'

s = tick[1]

s.time = 2
s.count = -1

s.callback = function(stage)
	print("+")
end

s:start()


