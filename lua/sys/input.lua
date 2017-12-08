--- input class
-- @module input
-- @alias Input
print('input.lua')
require 'norns'
norns.version.input = '0.0.1'

norns.key = function(n, val)
  print("key "..n..": "..val)
end

norns.enc = function(n, delta)
  print("enc "..n..": "..delta)
end

norns.battery = function(percent)
  print("battery: "..percent.."%")
end

norns.power = function(present)
  print("power: "..present)
end

