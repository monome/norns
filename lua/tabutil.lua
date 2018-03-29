--- table utility
-- @module tabutil

local tab = {}

--- print the contents of a table
-- @param t table to print
tab.print = function(t)
  for k,v in pairs(t) do print(k .. '\t' .. v) end
end

--- return a lexigraphically sorted array of keys for a table
-- @param t table to sort
-- @return sorted table
tab.sort = function(t)
  local keys = {}
  for k in pairs(t) do table.insert(keys, k) end
  table.sort(keys)
  return keys
end

--- count the number of entries in a table;
-- unlike table.getn() or #table, nil entries won't break the loop
-- @param t table to count
-- @return count
tab.count = function(t)
  local c = 0
  for _ in pairs(t) do c = c + 1 end
  return c
end

--- search table for element
-- @param t table to check
-- @param e element to look for
-- @return t/f is element is present
tab.contains = function(t,e)
  for index, value in ipairs(t) do
    if value == e then return true end
  end
  return false
end

--- split multi-line string into table of strings
-- @param str string with line breaks
-- @return table with entries for each line
tab.lines = function(str)
  local t = {}
  local function helper(line)
    table.insert(t, line)
    return ""
  end
  helper((str:gsub("(.-)\r?\n", helper)))
  return t
end

return tab
