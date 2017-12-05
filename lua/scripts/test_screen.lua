x = 15
y = 15
z = 15

norns.enc = function(n, delta)
	if(n==2) then x = x + delta end
	if(n==3) then y = y + delta end
	if(n==1) then z = z + delta end

	x = x % 128;
	y = y % 63;
	z = z % 16;

	screen_line(63,31,x,y,z)

end
