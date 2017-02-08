--[[ 
   sticksine.lua

   basic demo script showing how to connect inputs to audio params.

   this dumb instrument maps joystick buttons to pitch movements.

   the output is a single sinewave.

   the first 4 joystick buttons are each mapped to a pitch interval:
   pressing the button makes the pitch go up by that interval;
   releasing it makes the pitch go down.

   holding button 5 inverts the behavior of buttons 1-4.
   pressing button 6 resets the pitch to the fundamental.

   joystick axis 1 controls amplitude.

---- TODO:
   joystick axis 2 controls amplitude lag time.
   joystick axis 3 bends the base pitch.
   joystick axis 4 controls pitch lag time.
  
--]]

print('running sticksine.lua')


-- load the audio engine we want to use
load_engine('TestSine')


-- make a 'pitch' object to hold our data and methods
-- for calculating and updating the pitch of the sinewave
pitch = {
   ---- variables
   -- fundamental frequency
   base = 220,
   -- list of just intonation ratios
   scale = { 2, 3/2, 4/3, 5/4, 6/5 },   
   -- current output frequency
   hz = 220,

   ---- methods
   update = function ()
	  set_param("hz", pitch.hz)
	  print('hz ' .. pitch.hz)
   end,

   -- move the pitch up
   stepup = function (degree)
	  if pitch.scale[degree] ~= nil then
		 pitch.hz = pitch.hz * pitch.scale[degree]
		 pitch.update()
	  end
   end,

   -- move the pitch down
   stepdown = function (degree)
	  if pitch.scale[degree] ~= nil then
		 pitch.hz = pitch.hz / pitch.scale[degree]
		 pitch.update()
	  end
   end,

   -- reset to the base frequency
   reset = function ()
	  pitch.hz = pitch.base
	  pitch.update()
   end
}

-- similar but simpler for the amp parameter
amp = {
   val = 1.0,
   
   set = function(x)
	  print('amp ' .. x)
	  amp.val = x
	  set_param('amp', x)
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



-- lua lacks a switch statement.
-- use a function table instead
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

-- similarly for joystick axis functions
axfunc = {
   -- axis 1: amp
   function(val)
	  local dz = 2000 -- deadzone
	  local mag = math.abs(val)
	  local a = 0.0
	  if mag > dz then
		 a = (mag - dz) / (32768 - dz)
	  end
	  amp.set(a)
   end,

   -- axis 2: amp lag
   function(val)
	  -- TODO
   end,

   -- axis 3: pitch bend
   function(val)
	  -- TODO
   end,

   -- axis 4: pitch lag
   function(val)
	  -- TODO
   end   
}


-- finally, glue our functions to the handlers defined in norns.lua
joystick.button = function(stick, but, val)
   if type(butfunc[but]) == "function" then butfunc[but](val) end
end

joystick.axis = function(stick, ax, val)
   if type(axfunc[ax]) == "function" then axfunc[ax](val) end
end

