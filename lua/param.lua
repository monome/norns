--- Param class
-- @module paramset

local ControlSpec = require 'controlspec'

local Param = {}
Param.__index = Param

--- constructor
-- @param name of param
-- @param controlspec
-- @param formatter
function Param.new(name, controlspec, formatter)
  local p = setmetatable({}, Param)
  if not controlspec then controlspec = ControlSpec.unipolar() end
  p.name = name
  p.controlspec = controlspec
  p.formatter = formatter 
  p.action = function(x) end

  if controlspec.default then
    p.raw = controlspec:unmap(controlspec.default)
  else
    p.raw = 0
  end
  return p
end

--- get
-- returns mapped value
function Param:get()
  return self.controlspec:map(self.raw)
end

--- get_raw
-- get 0-1
function Param:get_raw()
  return self.raw
end

--- set
-- accepts a mapped value
function Param:set(value)
  self:set_raw(util.round(self.controlspec:unmap(value),controlspec.step))
end

--- set_raw
-- set 0-1
function Param:set_raw(value)
  clamped_value = util.clamp(value, 0, 1)
  if self.raw ~= clamped_value then
    self.raw = clamped_value
    self:bang()
  end
end

--- delta
-- add delta to current value. checks controlspec for mapped vs not
-- default division of delta for 100 steps range
function Param:delta(d)
  self:set_raw(self.raw + d/100)
end

--- set_default
function Param:set_default()
  self:set(self.controlspec.default)
end

--- bang
function Param:bang()
  self.action(self:get())
end

--- string
-- @return formatted string
function Param:string()
  if self.formatter then
    return self.formatter(self)
  else
  local a = util.round(self:get(), 0.01)
    return a.." "..self.controlspec.units
  end
end

return Param
