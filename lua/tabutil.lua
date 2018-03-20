local tab = {}

-- print the contents of a table
tab.print = function(t)
   for k,v in pairs(t) do print(k .. '\t' .. v) end
end

-- return a lexigraphically sorted array of keys for a table
tab.sort = function(t)
   local keys = {}
   for k in pairs(t) do table.insert(keys, k) end
   table.sort(keys)
   return keys
end

-- count the number of entries in a table
-- unlike table.getn() or #table, nil entries won't break the loop
tab.count = function(t)
  local c = 0
  for _ in pairs(t) do c = c + 1 end
  return c
end

-- return t/f if table contains element
tab.contains = function(t,e)
    for index, value in ipairs(t) do
        if value == e then return true end
    end
    return false
end

return tab
