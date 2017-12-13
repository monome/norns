--- power management
-- @section power

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

