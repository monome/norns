joystick.axis = function(stick, ax, val)
   -- shhh
end

joystick.button = function(stick, but, val)
   if(val > 0) then
	  if (but ==1) then
		 start_timer(1, 1.0, 32)
	  end
	  
	  if (but ==2) then
		 start_timer(1, 1.0, 32)
	  end
	  
	  if (but ==3) then
		 stop_timer(1)
	  end
   end
end
