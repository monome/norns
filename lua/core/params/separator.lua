-- Separator class
-- @classmod separator

local Separator = {}
Separator.__index = Separator

local tSEPARATOR = 0

function Separator.new(name)
  local s = setmetatable({}, Separator)
  s.name = name or ""
  s.t = tSEPARATOR
  s.action = function() end
  return s
end

function Separator:get()
  return ""
end

function Separator:set(v) end
function Separator:delta(d) end
function Separator:set_default() end
function Separator:bang() end

function Separator:string()
  return self.name
end

return Separator
