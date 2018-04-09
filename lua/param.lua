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


function Param:string(quant)
  if self.formatter then
    return self.formatter(self)
  else
    local mapped_value = self:get()
    local display_value
    if quant then
      display_value = util.round(mapped_value, quant)
    else
      display_value = mapped_value
    end
    return self:string_format(display_value)
  end
end

function Param:string_format(value, units, name)
  local u
  if units then
    u = units
  elseif self.controlspec then
    u = self.controlspec.units
  else
    u = ""
  end
  --return Param.stringify(name or self.name or "", u, value)
  return value.." "..u
end

function Param.stringify(name, units, value)
  return name..": "..value.." "..units
end


return Param
