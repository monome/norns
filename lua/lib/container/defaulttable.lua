-- DefaultTable - a table which provides a default value initialization for each key
-- @module DefaultTable
-- @alias DefaultTable

local function shallow_copy(t)
  -- MAINT: the {table.unpack(t)} trick doesn't work if the given table has
  -- non-integer keys or a numeric indicies with gaps
  local new = {}
  for k,v in pairs(t) do
    new[k] = v
  end
  return new
end

local DefaultTable = {}
DefaultTable.__index = DefaultTable

--- Create a table where every key is inialized with a default value when first
-- accessed.
--
-- Providing a table as the initial value will result in a shallow copy of the
-- given table being made when a key is initially accessed. Providing a (zero
-- argument) function will result in the function being called to obtain the
-- initial value. Providing any other value will result in that value being
-- used as the initial value for keys (which could result structural sharing
-- between keys).
--
-- @tparam anything initial Value initializer (can be nil).
-- @treturn table
function DefaultTable.new(initial)
  local self = {}
  self._table = {}

  local t = type(initial)
  if t == 'table' then
    self._initializer = function() return shallow_copy(initial) end
  elseif t == 'function' then
    self._initializer = initial
  else
    self._initializer = function() return initial end
  end

  setmetatable(self, DefaultTable)

  return self
end

function DefaultTable:__index(k)
  local _t = rawget(self, '_table')
  local v = _t[k]
  if v == nil then
    local _i = rawget(self, '_initializer')
    v = _i()
    _t[k] = v
  end
  return v
end

function DefaultTable:__newindex(k, v)
  rawget(self, '_table')[k] = v
end

function DefaultTable:__len()
  return #self._table
end

function DefaultTable.__pairs(this)
  return pairs(rawget(this, '_table'))
end

function DefaultTable.__ipairs(this)
  return ipairs(rawget(this, '_table'))
end

return DefaultTable
