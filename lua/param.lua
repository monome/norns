local Param = {}
Param.__index = Param

function Param.new(title, controlspec, formatter)
  local p = setmetatable({}, Param)
  p.title = title
  p.controlspec = controlspec
  p.formatter = formatter

  if controlspec and controlspec.default then
    p.value = controlspec:unmap(controlspec.default)
  else
    p.value = 0
  end
  return p
end

function Param:string(quant)
  if self.formatter then
    return self.formatter(self)
  else
    local mapped_value = self:mapped_value()
    local display_value
    if quant then
      display_value = util.round(mapped_value, quant)
    else
      display_value = mapped_value
    end
    return self:string_format(display_value)
  end
end

function Param:string_format(value, units, title)
  local u
  if units then
    u = units
  elseif self.controlspec then
    u = self.controlspec.units
  else
    u = ""
  end
  return Param.stringify(title or self.title or "", u, value)
end

function Param.stringify(title, units, value)
  return title..": "..value.." "..units
end

function Param:set(value)
  clamped_value = util.clamp(value, 0, 1)
  if self.value ~= clamped_value then
    prev_value = self.value
    self.value = clamped_value
    self:bang() -- TODO: this broke on_change(newvalue, prevvalue)
  end
end

function Param:bang()
  if self.on_change then -- TODO: i'm not fond of splitting up on_change / on_change_mapped, it would be wiser to just have on_change(param, old_vaule) or something
    self.on_change(self.value, self.value) -- TODO: this was first intended as on_change(newvalue, prevvalue) but is not working now
  end
  if self.on_change_mapped then
    local value_mapped = self.controlspec:map(self.value)
    self.on_change_mapped(value_mapped, value_mapped) -- TODO: this was first intended as on_change(newvalue, prevvalue) but is not working now
  end
end

function Param:set_mapped_value(value)
  self:set(self.controlspec:unmap(value))
end

function Param:adjust(delta)
  self:set(self.value + delta)
end

function Param:mapped_value()
  return self.controlspec:map(self.value)
end

function Param:revert_to_default()
  if self.controlspec and self.controlspec.default then
    self:set_mapped_value(self.controlspec.default)
  else
    self:set(0)
  end
end

return Param
