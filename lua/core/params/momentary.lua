-- Momentary class
-- @module toggle

local Momentary = {}
Momentary.__index = Momentary

local tMOMENTARY = 10

function Momentary.new(id, name, default)
  local o = setmetatable({}, Momentary)
  o.t = tMOMENTARY
  o.id = id
  o.name = name
  o.default = default or false
  o.value = o.default
  o.action = function() end
  return o
end

function Momentary:get()
  return self.value
end

function Momentary:set(v, silent)
  local silent = silent or false
  if self.value ~= v then
    self.value = v
    if silent==false then self:bang() end
  end
end

function Momentary:delta(d)
  if d ~= 0 then self:set(not self.value) end
end

function Momentary:set_default()
  self:set(self.default)
end

function Momentary:bang()
  self.action(self.value)
end

function Momentary:string()
   return self.value
end


return Momentary
