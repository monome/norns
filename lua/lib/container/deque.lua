-- deque - double ended queue
-- @module deque
-- @alias Deque

-- reference: https://www.lua.org/pil/11.4.html


local REMOVED = { 'deque removed value' }

local Deque = {}
Deque.__index = Deque

function Deque.new(elements)
  local o = setmetatable({}, Deque)
  o.first = 0
  o.last = -1
  o.tombstones = 0
  o._e = {}
  if elements ~= nil then
    for _, e in ipairs(elements) do
      o:push_back(e)
    end
  end
  return o
end

function Deque:push(value)
  local first = self.first - 1
  self.first = first
  self._e[first] = value
end

function Deque:push_back(value)
  local last = self.last + 1
  self.last = last
  self._e[last] = value
end

function Deque:extend_back(other_deque)
  for _, e in other_deque:ipairs() do
    self:push_back(e)
  end
end

function Deque:pop()
  local first = self.first
  if first > self.last then
    -- empty
    return nil
  end
  local value = self._e[first]
  self._e[first] = nil
  self.first = first + 1
  if value ~= REMOVED then
    return value
  end
  -- tail call to skip over tombstone
  self.tombstones = self.tombstones - 1
  return self:pop()
end

function Deque:pop_back()
  local last = self.last
  if self.first > last then
    -- empty
    return nil
  end
  local value = self._e[last]
  self._e[last] = nil
  self.last = last - 1
  if value ~= REMOVED then
    return value
  end
  -- tail call to skip over tombstone
  self.tombstones = self.tombstones - 1
  return self:pop_back()
end

local function _eq(a, b)
  return a == b
end

function Deque:contains(value, predicate)
  predicate = predicate or _eq
  for _, v in self:ipairs() do
    if predicate(v, value) then
      return true
    end
  end
  return false
end

function Deque:remove(value, predicate)
  predicate = predicate or _eq

  -- optimal case (match head or tail)
  if predicate(self._e[self.first], value) then
    return self:pop()
  elseif predicate(self._e[self.last], value) then
    return self:pop_back()
  end

  -- search for a match and tombstone it
  local i = self.first
  while i <= self.last do
    local e = self._e[i]
    if e ~= REMOVED and predicate(e, value) then
      self._e[i] = REMOVED
      self.tombstones = self.tombstones + 1
      return e
    end
    i = i + 1
  end

  return nil
end

function Deque:count()
  if self.last < self.first then
    return 0
  end
  return self.last - self.first - self.tombstones + 1
end

function Deque:clear()
  self._e = {}
  -- must match new()
  self.first = 0
  self.last = -1
  self.tombstones = 0
end

function Deque:ipairs()
  local first = self.first
  local last = self.last
  local i = first
  local n = 0
  local f

  f = function()
    if i > last then
      return nil
    end
    local element = self._e[i]
    i = i + 1
    if element ~= REMOVED then
      n = n + 1
      return n, element
    end
    -- tail call to skip tombbstone
    return f()
  end

  return f
end

function Deque:to_array()
  local r = {}
  for i, e in self:ipairs() do
    r[i] = e
  end
  return r
end

return Deque




