numbers = {0,0,0,0,0}

enc = function(n, delta)
	if(n==2) then 
        numbers[1] = numbers[1] + delta
    elseif(n==3) then 
        numbers[2] = numbers[2] + delta
    end 
    redraw()
end

key = function(n, z)
	if n==2 then
        numbers[3] = z
    elseif n==3 then
        numbers[4] = z
	end
    redraw()
end

local metro = require('metro')
c = metro[1]
c.count = -1
c.time = 1
c.callback = function(stage)
    numbers[5] = numbers[5] + 1
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

c:start()
