--- gpio class
-- @module gpio
-- @alias GPIO
print('gpio.lua')
require 'norns'
norns.version.gpio = '0.0.1'

norns.gpio = function(pin, val)
  print("gpio "..pin..": "..val)
end

