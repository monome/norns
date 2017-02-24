--[[ 
   sticksine.lua

   basic demo script showing how to connect inputs to audio params.

   this dumb instrument maps joystick buttons to pitch movements (tested with xbox360 controller.)

   the output is a single sinewave

   the first 4 joystick buttons are each mapped to a pitch interval:
   pressing the button makes the pitch go up by that interval;
   releasing it makes the pitch go down.

   holding button 5 inverts the behavior of buttons 1-4.
   pressing button 6 resets the pitch to the base.

   joystick axis 3 controls amplitude.
   joystick axis 4 bends the base pitch.
  
   holding 'up' on the d-pad engages a timer to randomly walk through pitch intervals.

--]]

print('running sticksine.lua')

-- load the audio engine we want to use
load_engine('TestSine')


-- make a 'pitch' object to hold data and methods
-- for calculating and updating the pitch of the sinewave
pitch = {
   ---- variables
   -- fundamental frequency
   base = 220,
   -- list of just intonation ratios
   scale = { 3/2, 4/3, 5/4, 6/5, 7/6 },
   -- current interval from base (as a ratio)
   ratio = 1.0,
   -- current output frequency
   hz = 220,
   ---- methods
   update = function ()
	  pitch.hz = pitch.ratio * pitch.base
	  e.hz(pitch.hz)
	  print('hz ' .. pitch.hz)
   end,
   -- move the pitch up
   stepup = function (degree)
	  if pitch.scale[degree] ~= nil then
		 pitch.ratio = pitch.ratio * pitch.scale[degree]
		 pitch.update()
	  end
   end,
   -- move the pitch down
   stepdown = function (degree)
	  if pitch.scale[degree] ~= nil then
		 pitch.ratio = pitch.ratio / pitch.scale[degree]
		 pitch.update()
	  end
   end,
   -- reset to the base frequency
   reset = function ()
	  print("pitch reset")
	  pitch.ratio = 1.0
	  pitch.update()
   end,
   -- take a random pitch step
   random = function()
	  if math.random() > 0.5 then
		 pitch.stepup(math.random(#(pitch.scale)))
	  else
		 pitch.stepdown(math.random(#(pitch.scale)))
	  end
   end
}

-- similar but simpler for the amp parameter
amp = {
   val = 1.0,
   set = function(x)
	  print('amp ' .. x)
	  amp.val = x
	  e.amp(x)	  
   end
}

-- simple variable for invert-button state
local invert_but = false

-- a function factory for our pitch-changing buttons
pitchbutfunc = function(but)
   return function(val)
	  if (val > 0) == invert_but then
		 pitch.stepdown(but);
	  else
		 pitch.stepup(but);
	  end
   end
end


-- lua lacks a switch statement;
-- function tables are one idiomatic alternative
butfunc = {
   -- buttons 1-4: change pitch
   pitchbutfunc(1),
   pitchbutfunc(2),
   pitchbutfunc(3),
   pitchbutfunc(4),
   -- button 5: invert
   function(val)
	  invert_but = not (val == 0)
   end,
   -- button 6: reset
   function(val)
	  if val then pitch.reset() end
   end
}

-- helper to map joystick axis value to unit value, with deadzone
function map_axis(x, bipolar)
   local y = 0.0
   local dz = 2000
   local mag = math.abs(x)
   if(bipolar) then -- map [-32768, 32768] to [-1, 1]
	  if mag > dz then
		 if(x < 0) then
			y = ( x + dz) / (32768 - dz)
		 else
			y = ( x - dz) / (32767 - dz)
		 end
	  end
   else -- map [-32768, 32768] to [0, 1]
	  if x > (-32768 + dz) then
		 y = (x + 32768 - dz) / (32767 + 32768 - dz)
	  else
		 y = 0
	  end
   end
   if y > 1.0 then y = 1.0 end
   if y < -1.0 then y = -1.0 end
   return y
end
   

-- similarly for joystick axis functions
axfunc = {
   -- axis 1
   function(val)
   end,
   -- axis 2
   function(val)
   end,
   -- axis 3 (amp)
   function(val)
	  amp.set(map_axis(val, false));
   end,
   -- axis 4: bend the pitch base
   function(val)
	  pitch.base = 220.0 * (2 ^ (map_axis(val, true)))
	  pitch.update()
   end   
}

-- finally, glue our function tables to the handlers defined in norns.lua
joystick.button = function(stick, but, val)
   if type(butfunc[but]) == "function" then butfunc[but](val) end
end

joystick.axis = function(stick, ax, val)
   if type(axfunc[ax]) == "function" then axfunc[ax](val) end
end

-- assign d-pad (aka 'hat') to timer stop/start
-- each direction is represented as a bitfield
-- so we need to store the state and compare 
local hat_state = false
joystick.hat = function(stick, hat, val)
   if (val & 0x1) > 0 then
	  if not hat_state then
		 hat_state = true
		 -- start timer 1 with a period of 1/8s, infinite repeats
		 start_timer(1, 0.125, -1)
	  end
   else
	  if hat_state then
		 hat_state = false
		 -- stop timer 1
		 stop_timer(1)
	  end
   end
end

-- define a timer callback
timer = function(idx, count)
   if idx == 1 then -- respond to timer 1
	  pitch.random()
   end
end
