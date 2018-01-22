-- template
-- a very basic example


-- specify dsp engine to load:
engine = 'TestSine'

-- init function
init = function(commands, count)
    print("template!")
    -- show engine commands available
    print("commands: ")
    for i,v in pairs(commands) do
        print(i, v.fmt)
    end
    -- set engine params
    e.hz(100)
    e.amp(0.125)
    -- start timer
    c:start()
    -- clear grid, if it exists
    if g then 
        g:all(1) 
        g:refresh()
    end 
end

-- make an array for storing
numbers = {0,0,0,0,0}
-- make a var, led brightness for grid
level = 5

-- encoder function
enc = function(n, delta)
    numbers[n] = numbers[n] + delta
    -- redraw screen
    redraw()
end

-- key function
key = function(n, z)
    numbers[n+2] = z
    -- redraw screen
    redraw()
end

-- screen redraw function
redraw = function()
    -- clear screen
    s.clear()
    -- set pixel brightness (0-15)
    s.level(15)

    for i=1,5 do
        -- move cursor
	    s.move(0,i*8)
        -- draw text
        s.text("> "..numbers[i])
    end 
end

-- set up a metro
c = metro[1]
-- count forever
c.count = -1
-- count interval to 1 second
c.time = 1
-- callback function on each count
c.callback = function(stage)
    numbers[5] = numbers[5] + 1
    redraw()
end

-- grid key function
gridkey = function(x, y, state)
   if state > 0 then 
      -- turn on led
      g:led(x, y, level)
   else
      -- turn off led
      g:led(x, y, 0)
   end
   -- refresh grid leds
   g:refresh()
end 

-- called on script quit, release memory
cleanup = function()
    numbers = nil
end
