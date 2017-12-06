math.randomseed(os.time())

x = 15
y = 15
z = 15
aa = 1

s_clear()
s_line_width(1.0)

norns.enc = function(n, delta)
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

norns.key = function(n, z)
	if(n==1 and z==1) then
		aa = 1 - aa
		s_aa(aa)
		s_clear()
	end
	if(n==2 and z==1) then 
		s_move(math.random()*128,math.random()*64)
		s_level(15)
		s_text("skull")
	end
	if(n==3 and z==1) then
		s_move(math.random()*128,math.random()*64)
		s_level(0)
		s_text("SKULL")
	end

end

