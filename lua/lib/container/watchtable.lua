-- WatchTable - a table which can be watched of key changes
-- @module WatchTable
-- @alias WatchTable

local WatchTable = {}
WatchTable.__index = WatchTable

--- Create a table which invokes a callback each time a value is set.
--
-- The callback fruction is passed the key, value after a given set operation
-- has been performed.
--
-- @tparam table t Initial table contents (can be nil).
-- @tparam function cb Callback on key set
-- @treturn table
function WatchTable.new(t, cb)
  local o = {}
  o.table = t or {}
  o.callback = cb or function(...) end
  setmetatable(o, WatchTable)
  return o
end

function WatchTable:__newindex(k, v)
  rawset(self.table, k, v)
  self.callback(k, v)
end

function WatchTable:__index(k)
  return self.table[k]
end

return WatchTable
