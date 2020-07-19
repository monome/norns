-- weaktable - a table with weakly held keys
-- @module weaktable
-- @alias WeakTable

local WeakTable = {}
WeakTable.__index = WeakTable
WeakTable.__mode = "k"  --  invoke dark magic: https://www.lua.org/pil/17.html

--- Create a table with weakly held keys
--
-- A table with weakly held keys allows objects to be used as keys within the
-- table but it won't prevent those objects from being garbage collected
--
-- @tparam table initial Initial table contents, optional.
-- @treturn table
function WeakTable.new(t)
  local t = setmetatable(t or {}, WeakTable)
  return t
end

return WeakTable
