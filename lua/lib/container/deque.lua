-- deque - double ended queue
-- @module deque
-- @alias Deque

-- reference: https://www.lua.org/pil/11.4.html


local REMOVED = { 'deque removed value' }

local Deque = {}
Deque.__index = Deque

--- Create a Deque value (object).
--
-- A double ended queue supporting O(1) insertion at the head or tail of the queue.
--
-- @tparam table elements Initial values to shallow copy into queue [optional]
-- @treturn Deque instance
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

--- Push a value onto the front of the queue
-- @tparam anything value The value to insert
function Deque:push(value)
  local first = self.first - 1
  self.first = first
  self._e[first] = value
end

--- Push a value onto the back of the queue
-- @tparam anything value The value to insert
function Deque:push_back(value)
  local last = self.last + 1
  self.last = last
  self._e[last] = value
end

--- Push all values from another Deque onto theback of this one
-- @tparam Deque other_deque The values to insert
function Deque:extend_back(other_deque)
  for _, e in other_deque:ipairs() do
    self:push_back(e)
  end
end

--- Pop the value off the front of the queue and return it
-- @treturn anything
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

-- Peek at the value n elements from the front
-- @treturn anything
function Deque:peek(n)
  local pos = self.first + (n or 1) - 1
  if pos > self.last then
    return nil -- empty
  end
  while pos <= self.last do
    local value = self._e[pos]
    if value ~= REMOVED then return value end
    pos = pos + 1
  end
  return nil
end

--- Pop the value off the back of the queue and return it
-- @treturn anything
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

-- Peek at the value n elements from the back
-- @treturn anything
function Deque:peek_back(n)
  local pos = self.last - ((n or 1) - 1)
  if pos < self.first then
    return nil -- empty
  end
  while pos >= self.first do
    local value = self._e[pos]
    if value ~= REMOVED then return value end
    pos = pos - 1
  end
  return nil
end

local function _eq(a, b)
  return a == b
end

--- Returns the queued value if the a match is found.
--
-- The optional predicate function should take two arguments and return true
-- if the arguments are considered a match. The default predicate is ==.
--
-- @tparam anything value The value to search the queue for
-- @tparam function predicate Comparison function [optional]
-- @treturn anything
function Deque:find(value, predicate)
  predicate = predicate or _eq
  for _, v in self:ipairs() do
    if predicate(v, value) then
      return v
    end
  end
  return nil
end

--- Returns true if the given value is in the queue.
--
-- The optional predicate function should take two arguments and return true
-- if the arguments are considered a match. The default predicate is ==.
--
-- @tparam anything value The value to search the queue for
-- @tparam function predicate Comparison function [optional]
-- @treturn boolean
function Deque:contains(value, predicate)
  return self:find(value, predicate) ~= nil
end

--- Removes the first instance over value in the queue
--
-- The optional predicate function should take two arguments and return true
-- if the arguments are considered a match. The default predicate is ==.
--
-- @tparam anything value The value to remove from the queue
-- @tparam function predicate Comparison function [optional]
-- @treturn anything Removed value or nil
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

--- Return the number of elements/values in the queue
-- @treturn int
function Deque:count()
  if self.last < self.first then
    return 0
  end
  return self.last - self.first - self.tombstones + 1
end

--- Clear out all elements in the queue
function Deque:clear()
  self._e = {}
  -- must match new()
  self.first = 0
  self.last = -1
  self.tombstones = 0
end

--- Returns an iterator for the queue which behaves like ipairs
-- @treturn function
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

--- Return a list/array with the elements of the queue (here head is index 1)
function Deque:to_array()
  local r = {}
  for i, e in self:ipairs() do
    r[i] = e
  end
  return r
end

return Deque




