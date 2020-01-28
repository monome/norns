--- Control class
-- @classmod control

local ControlSpec = require 'core/controlspec'

local Control = {}
Control.__index = Control

local tCONTROL = 3

--- constructor.
-- @param id
-- @param name
-- @param controlspec
-- @param formatter
function Control.new(id, name, controlspec, formatter)
  local p = setmetatable({}, Control)
  p.t = tCONTROL
  if not controlspec then controlspec = ControlSpec.UNIPOLAR end
  p.id = id
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

--- get.
-- returns mapped value.
function Control:get()
  return self.controlspec:map(self.raw)
end

--- get_raw.
-- get 0-1.
function Control:get_raw()
  return self.raw
end

--- set.
-- accepts a mapped value
function Control:set(value, silent)
  self:set_raw(self.controlspec:unmap(util.round(value,self.controlspec.step)), silent)
end

--- set_raw.
-- set 0-1.
function Control:set_raw(value, silent)
  local silent = silent or false
  local clamped_value = util.clamp(value, 0, 1)
  if self.raw ~= clamped_value then
    self.raw = clamped_value
    if silent==false then self:bang() end
  end
end

--- delta.
-- add delta to current value. checks controlspec for mapped vs not.
-- default division of delta for 100 steps range.
function Control:delta(d)
  self:set_raw(self.raw + d/100)
end

--- set_default.
function Control:set_default()
  self:set(self.controlspec.default)
end

--- bang.
function Control:bang()
  self.action(self:get())
end

--- string.
-- @return formatted string
function Control:string()
  if self.formatter then
    return self.formatter(self)
  else
  local a = util.round(self:get(), 0.01)
    return a.." "..self.controlspec.units
  end
end

return Control
