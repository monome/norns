require 'norns'
local input = require 'input'

local pad = nil
local butCode = 'BTN_SOUTH'

-- on startup, see if there's already a gamepad connected
pad = input.findDeviceSupporting('EV_KEY', butCode)

-- when a new device is added, see if its a gamepad
input.add = function (device)
   if device:supports('EV_KEY', butCode) then
      pad = device
      print("grabbing new device: ")
      device:print()      
   end
end

-- print everything about every input event from the gamepad
input.event = function(id, ev_type, ev_code, value)
   if id == pad.id then
      print ("test_input_devices: got event")
      print (input.event_types[ev_type], input.event_codes[ev_type][ev_code], value)
   end
end

--- cleanup function
norns.cleanup = function()
   pad = nil
   butCode = nil
end
