--16sliders

math.randomseed(os.time())

sliders = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
edit = 1
accum = 1
step = 0

e = require 'engine' 
e.load('TestSine',function(commands,count)
            print("loaded")
            e.hz(100)
            k:start()
    end
)


local metro = require('metro')
k = metro[1]
k.count = -1
k.time = 0.1
k.callback = function(stage)
    step = (step + 1) % 16
    e.hz(2^(sliders[step+1]/12) * 80)
    redraw()
end 

norns.script.cleanup = function()
    k:stop()
end

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
        sliders[1] = math.random()*4
        for i=2,16 do
            sliders[i] = sliders[i-1]+math.floor(math.random()*9)-3
        end
		redraw()
    elseif(n==2 and z==1) then 
        for i=1,16 do
            sliders[i] = sliders[i]+math.floor(math.random()*5)-2
        end
        redraw()
	end
end

redraw = function()
    s.aa(1)
    s_line_width(1.0)
    s.clear()

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

    s.level(10)
    s.move(32+step*4,50)
    s.line(32+step*4,54)
    s.stroke()
end
