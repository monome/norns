math.randomseed(os.time())

sliders = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
edit = 1
accum = 1
step = 0
aa = 0

s_aa(aa)
s_line_width(1.0)

enc = function(n, delta)
	if(n==2) then 
        accum = (accum + delta) % 64
        edit = accum >> 2 
    elseif(n==3) then 
        sliders[edit+1] = sliders[edit+1] + delta
        if sliders[edit+1] > 32 then sliders[edit+1] = 32 end
        if sliders[edit+1] < 0 then sliders[edit+1] = 0 end 
    end 
    redraw()
end

key = function(n, z)
	if(n==3 and z==1) then
		aa = 1 - aa
		s.aa(aa)
		redraw()
    elseif(n==2 and z==1) then 
        step = 0
        redraw()
	end
end

local metro = require('metro')
k = metro[1]
k.count = -1
k.time = 0.25
k.callback = function(stage)
    step = (step + 1) % 16
    redraw()
end

redraw = function()
    s.clear()
    s.level(15)
    s.move(0,63)
    s.text("norns")

    for i=0,15 do
	    if(i==edit) then
            s.level(15)
        else
            s.level(2)
        end
	    s.move(32+i*4,48)
	    s.line(32+i*4,46-sliders[i+1])
	    s.stroke()
    end

    s.move(32+step*4,50)
    s.line(32+step*4,52)
    s.stroke()
end

init = function()
    k:start()
end
