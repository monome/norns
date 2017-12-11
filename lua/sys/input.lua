--- input class
-- @module input

print('input.lua')
require 'norns'
norns.version.input = '0.0.1'

--- redefine key handler
-- @param n key number
-- @param val value (0=up,1=down)
norns.key = function(n, val)
  print("key "..n..": "..val)
end

--- redefine enc handler
-- @param n encoder number
-- @param delta delta (negative=CCW,positive=CW)
norns.enc = function(n, delta)
  print("enc "..n..": "..delta)
end

--- redefine battery percent handler
-- @param percent battery full percentage
norns.battery = function(percent)
  print("battery: "..percent.."%")
end

--- redefine power present handler
-- @param present power plug present (0=no,1=yes)
norns.power = function(present)
  print("power: "..present)
end

