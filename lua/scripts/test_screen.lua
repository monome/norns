math.randomseed(os.time())

x = 15
y = 15
z = 15
aa = 1

s.clear()
s_line_width(1.0)

enc = function(n, delta)
	if(n==2) then x = x + delta end
	if(n==3) then y = y + delta end
	if(n==1) then z = z + delta end

	x = x % 128;
	y = y % 64;
	z = z % 16;

	s_level(z)
	s_move(63,31)
	s_line(x,y)
	s_stroke()
	--screen_line(63,31,x,y,z)
end

key = function(n, z)
	if(n==3 and z==1) then
		aa = 1 - aa
		s_aa(aa)
		redraw()
    elseif(n==2 and z==1) then 
		s_move(math.random()*128,math.random()*64)
		s_level(15)
		s_text("skull")
	end
end

local metro = require('metro')
k = metro[1]
k.count = -1
k.time = 0.25
k.callback = function(stage)
    s_move(math.random()*128,math.random()*64)
	s_level(2)
	s.text("*")
end
k:start()

redraw = function()
    s.clear()
    s.move(0,63)
    s.text("norns")
end
