print('test_input_devices.lua')
require 'norns'
local input = require 'input'

--- local variables
local pad = nil
local callbacks = {
   BTN_NORTH = function(val) print("B.N: ", val) end,
   BTN_SOUTH = function(val) print("B.S: ", val) end,
   BTN_EAST = function(val) print("B.E: ", val) end,
   BTN_WEST = function(val) print("B.W: ", val) end,
   ABS_X = function(val) print("L.X: ", val) end,
   ABS_Y = function(val) print("L.Y: ", val) end,
   ABS_RX = function(val) print("R.X: ", val) end,
   ABS_RY = function(val) print("R.Y: ", val) end
}
      
norns.cleanup = function()
   if pad then pad:clearCallbacks() end
   pad = nil
   callbacks = nil
end

local setPad = function(device)
   print("grabbing device: ")
   device:print()
   -- stop any callbacks we may have added to the last device we used
   if pad then
      print("clearing old callbacks")
      pad:clearCallbacks()
   end   
   -- use the new device
   pad = device
   for code,func in pairs(callbacks) do
      print(code, func)
      pad.callbacks[code] = func
   end

end

-- on startup, see if there's already a gamepad connected
pad = input.findDeviceSupporting('EV_KEY', 'BTN_START') -- << FIXME, shouldn't need ev type
if pad then setPad(pad) end

-- when a new device is added, see if its a gamepad
input.add = function (device)
   if device:supports('EV_KEY', 'BTN_START') then
      setPad(device)
   end
end
