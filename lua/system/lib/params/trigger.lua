--- Trigger class
-- @module trigger

local Trigger = {}
Trigger.__index = Trigger

local tTRIGGER = 6

function Trigger.new(id, name)
  local o = setmetatable({}, Trigger)
  o.t = tTRIGGER
  o.id = id
  o.name = name
  o.action = function() end
  return o
end

function Trigger:get()
  return nil
end

function Trigger:set(v)
  self:bang()
end

function Trigger:delta(d)
  --noop
end

function Trigger:set_default()
  --noop
end

function Trigger:bang()
  self.action()
end

function Trigger:string()
  return ""
end


return Trigger
