-- Toggle class
-- @module toggle

local Toggle = {}
Toggle.__index = Toggle

local tTOGGLE = 9

function Toggle.new(id, name, default)
  local o = setmetatable({}, Toggle)
  o.t = tTOGGLE
  o.id = id
  o.name = name
  o.default = default or false
  o.value = o.default
  o.action = function() end
  return o
end

function Toggle:get()
  return self.value
end

function Toggle:set(v, silent)
  local silent = silent or false
  if self.value ~= v then
    self.value = v
    if silent==false then self:bang() end
  end
end

function Toggle:delta(d)
  if d ~= 0 then self:set(not self.value) end
end

function Toggle:set_default()
  self:set(self.default)
end

function Toggle:bang()
  self.action(self.value)
end

function Toggle:string()
   return self.value
end


return Toggle
