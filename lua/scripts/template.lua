engine = 'TestSine'

init = function(commands, count)
    print("commands: ")
    for i,v in pairs(commands) do
        print(i, v.fmt)
    end
    e.hz(100)
    e.amp(0.125)
    c:start()
end

numbers = {0,0,0,0,0}

enc = function(n, delta)
    numbers[n] = numbers[n] + delta
    redraw()
end

key = function(n, z)
    numbers[n+2] = z
    redraw()
end

redraw = function()
    s.clear()
    s.level(15)

    for i=1,5 do
	    s.move(0,i*8)
        s.text("> "..numbers[i])
    end 
end

--local metro = require('metro')
c = metro[1]
c.count = -1
c.time = 1
c.callback = function(stage)
    numbers[5] = numbers[5] + 1
    redraw()
end

cleanup = function()
    numbers = nil
end
