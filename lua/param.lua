local ControlSpec = require 'controlspec'

local Param = {}
Param.__index = Param

function Param.new(name, controlspec, formatter)
  local p = setmetatable({}, Param)
  if not controlspec then controlspec = ControlSpec.default() end
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


function Param:get()
  return self.controlspec:map(self.raw)
end

function Param:get_raw()
  return self.raw
end

function Param:set(value)
  self:set_raw(self.controlspec:unmap(value))
end

function Param:set_raw(value)
  clamped_value = util.clamp(value, 0, 1)
  if self.raw ~= clamped_value then
    self.raw = clamped_value
    self:bang()
  end
end

function Param:delta(d)
  if self.controlspec.delta_map then
    self:delta_raw(d/self.controlspec.delta_div)
  else
    self:set(self:get() + d/self.controlspec.delta_div)
  end
end

function Param:delta_raw(d)
  self:set_raw(self.raw + d)
end

function Param:set_default()
  if self.controlspec.default then
    self:set(self.controlspec.default)
  else
    self:set(0)
  end
end

function Param:bang()
  self.action(self:get())
end


function Param:string()
  if self.formatter then
    return self.formatter(self)
  else
    local a = util.round(self:get(), self.controlspec.display_prec)
    return a.." "..self.controlspec.units
  end
end

return Param
