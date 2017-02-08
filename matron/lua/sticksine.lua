--[[ 
   sticksine.lua

   basic demo script showing how to connect inputs to audio params.

   this little instrument maps joystick buttons to pitch movements.

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
local pitch = {
   ---- variables
   -- fundamental frequency
   base = 220,
   -- list of just intonation ratios
   scale = { 2, 3/2, 4/3, 5/4, 6/5 },   
   -- current output frequency
   hz = 220
}

print('pitch object: ' .. pitch)


---- methods
-- update the synthesizer with the current pitch
function pitch:update()
   set_param("hz", pitch.hz)
end

-- move the pitch up
function pitch:stepup(degree)
   if pitch.scale[degree] ~= nil then
	  pitch.hz = pitch.hz * pitch.scale[degree]
	  pitch.update()
   end
end


-- move the pitch down
function pitch:stepdown(degree)
   if pitch.scale[degree] ~= nil then
	  pitch.hz = pitch.hz / pitch.scale[degree]
	  pitch.update()
   end
end

-- reset to the base frequency
function pitch:reset()
--   pitch.hz = pitch.base
   pitch.update()
end

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

--[[
-- lua lacks a switch statement.
-- use a function table instead
butfunc = {
   1 = pitchbutfunc(1),
   2 = pitchbutfunc(2),
   3 = pitchbutfunc(3),
   4 = pitchbutfunc(4),

   5 = function(val)
	  if val then invert_but = true
	  else invert_but = false
	  end
   end,

   6 = function(val)
	  if val then pitch.reset() end
   end
}
--]]


print('butfunc: ' .. butfunc)

-- similarly for joystick axis functions
--[[
axfunc = {
   1 = function(val)
	  print('ax 1 '.. val)
	  local dz = 2000 -- deadzone
	  local mag = val.abs
	  local a = 0.0
	  if mag > dz then
		 a = (mag - dz) / (32768 - dz)
	  end
	  amp.set(a)
   end,
   
   2 = function(val)
	  print('ax 2 '.. val)
	  -- TODO
   end,

   3 = function(val)
	  print('ax 3 '.. val)
	  -- TODO
   end,

   1 = function(val)
	  print('ax 4 '.. val)
	  -- TODO
   end   
}

		 --]]


-- finally, glue our functions to the handlers defined in norns.lua
joystick.button = function(stick, but, val)
   print('but')
--   if type(butfunc[but]) == "function" then butfunc[but](val) end
end

joystick.axis = function(stick, ax, val)
      print('ax')
   -- if type(axfunc[ax]) == "function" then axfunc[ax](val) end
end

