--- table utility
--
-- The [norns script reference](https://monome.org/docs/norns/reference/)
-- has [examples for this module](https://monome.org/docs/norns/reference/lib/tabutil).
--
-- @module lib.tabutil
-- @alias tab

local tab = {}

--- print the contents of a table
-- @tparam table t table to print
tab.print = function(t)
  for k,v in pairs(t) do print(k .. '\t' .. tostring(v)) end
end

--- return a lexigraphically sorted array of keys for a table
-- @tparam table t table to sort
-- @treturn table sorted table
tab.sort = function(t)
  local keys = {}
  for k in pairs(t) do table.insert(keys, k) end
  table.sort(keys)
  return keys
end

--- count the number of entries in a table;
-- unlike table.getn() or #table, nil entries won't break the loop
-- @tparam table t table to count
-- @treturn number count
tab.count = function(t)
  local c = 0
  for _ in pairs(t) do c = c + 1 end
  return c
end

--- search table for element
-- @tparam table t table to check
-- @param e element to look for
-- @treturn boolean t/f is element is present
tab.contains = function(t,e)
  for index, value in ipairs(t) do
    if value == e then return true end
  end
  return false
end


--- given a simple table of primitives, 
--- "invert" it so that values become keys and vice versa.
--- this allows more efficient checks on multiple values
-- @param t a simple table
tab.invert = function(t)
  local inv = {}
  for k,v in pairs(t) do
    inv[v] = k
  end
  return inv
end


--- search table for element, return key
-- @tparam table t table to check
-- @param e element to look for
-- @return key, nil if not found
tab.key = function(t,e)
  for index, value in ipairs(t) do
    if value == e then return index end
  end
  return nil
end

--- split multi-line string into table of strings
-- @tparam string str string with line breaks
-- @treturn table table with entries for each line
tab.lines = function(str)
  local t = {}
  local function helper(line)
    table.insert(t, line)
    return ""
  end
  helper((str:gsub("(.-)\r?\n", helper)))
  return t
end


--- split string into table with delimiter
-- @tparam string inputstr : string to split
-- @tparam string sep : delimiter
tab.split = function(inputstr, sep)
	if sep == nil then
		sep = "%s"
	end
	local t={}
	for str in string.gmatch(inputstr, "([^"..sep.."]+)") do
		table.insert(t, str)
	end
	return t
end


--- Save a table to disk.
-- Saves tables, numbers, booleans and strings.
-- Inside table references are saved.
-- Does not save userdata, metatables, functions and indices of these.
-- Based on http://lua-users.org/wiki/SaveTableToFile by ChillCode.
-- @tparam table tbl Table to save.
-- @tparam string filename Location to save to.
-- @return On failure, returns an error msg.
function tab.save(tbl, filename)
  local charS, charE = "   ", "\n"
  local file, err = io.open(filename, "wb")
  if err then return err end

  -- initiate variables for save procedure
  local tables, lookup = { tbl }, { [tbl] = 1 }
  file:write("return {"..charE)

  for idx, t in ipairs(tables) do
    file:write("-- Table: {"..idx.."}"..charE)
    file:write("{"..charE)
    local thandled = {}

    for i, v in ipairs(t) do
      thandled[i] = true
      local stype = type(v)
      -- only handle value
      if stype == "table" then
        if not lookup[v] then
          table.insert(tables, v)
          lookup[v] = #tables
        end
        file:write(charS.."{"..lookup[v].."},"..charE)
      elseif stype == "string" then
        file:write(charS..string.format("%q", v)..","..charE)
      elseif stype == "number" then
        file:write(charS..tostring(v)..","..charE)
      elseif stype == "boolean" then
        file:write(charS..tostring(v)..","..charE)
      end
    end

    for i, v in pairs(t) do
      -- escape handled values
      if (not thandled[i]) then

        local str = ""
        local stype = type(i)
        -- handle index
        if stype == "table" then
          if not lookup[i] then
             table.insert(tables, i)
             lookup[i] = #tables
          end
          str = charS.."[{"..lookup[i].."}]="
        elseif stype == "string" then
          str = charS.."["..string.format("%q", i).."]="
        elseif stype == "number" then
          str = charS.."["..tostring(i).."]="
        elseif stype == "boolean" then
          str = charS.."["..tostring(i).."]="
        end

        if str ~= "" then
          stype = type(v)
          -- handle value
          if stype == "table" then
            if not lookup[v] then
              table.insert(tables, v)
              lookup[v] = #tables
            end
            file:write(str.."{"..lookup[v].."},"..charE)
          elseif stype == "string" then
            file:write(str..string.format("%q", v)..","..charE)
          elseif stype == "number" then
            file:write(str..tostring(v)..","..charE)
          elseif stype == "boolean" then
            file:write(str..tostring(v)..","..charE)
          end
        end
      end
    end
    file:write("},"..charE)
  end
  file:write("}")
  file:close()
end

--- Load a table that has been saved via the tab.save() function.
-- @tparam string sfile Filename or stringtable to load.
-- @return On success, returns a previously saved table. On failure, returns as second argument an error msg.
function tab.load(sfile)
  local ftables,err = loadfile(sfile)
  if err then return _, err end
  local tables = ftables()
  if tables ~= nil then
    for idx = 1, #tables do
      local tolinki = {}
      for i, v in pairs(tables[idx]) do
        if type(v) == "table" then
          tables[idx][i] = tables[v[1]]
        end
        if type(i) == "table" and tables[i[1]] then
          table.insert(tolinki, { i, tables[i[1]] })
        end
      end
      -- link indices
      for _, v in ipairs(tolinki) do
        tables[idx][v[2]], tables[idx][v[1]] =  tables[idx][v[1]], nil
      end
    end
    return tables[1]
  else
    return nil
  end
end

--- Create a read-only proxy for a given table.
-- @param params params.table is the table to proxy, params.except a list of writable keys, params.expose limits which keys from params.table are exposed (optional)
-- @treturn table the proxied read-only table
function tab.readonly(params)
  local t = params.table
  local exceptions = params.except or {}
  local proxy = {}

  local proxy_index = function(_, k)
    if params.expose == nil or tab.contains(params.expose, k) then
      return t[k]
    end
    return nil
  end

  local proxy_newindex = function (_, k, v)
    if (tab.contains(exceptions, k)) then
      t[k] = v
    else
      error("'"..k.."', a read-only key, cannot be re-assigned.")
    end
  end

  local proxy_pairs = function(_)
    local iter = function(_, key)
      for k, v in next, t, key do
        if proxy_index(nil, k) ~= nil then
          return k, v
        end
        _, _ = next(t, k)
      end
      return nil
    end
    return iter, t, nil
  end

  local proxy_ipairs = function(_)
    local iter = function()
      for i = 1, tab.count(t) do
        if proxy_index(nil, i) ~= nil then
          return i, t[i]
        end
      end
      return nil
    end
    return iter
  end

  local mt = {
    __index = proxy_index,
    __newindex = proxy_newindex,
    __pairs = proxy_pairs,
    __ipairs = proxy_ipairs,
  }
  setmetatable(proxy, mt)
  return proxy
end


--- return new table, gathering values:
--- - first from default_values, 
--- - then from (i.e. overridden by) custom_values
--- nils in custom_values are ignored
-- @tparam table default_values base values (provides keys & fallback values)
-- @tparam table custom_values override values (take precedence)
-- @treturn table composite table
function tab.gather(default_values, custom_values)
  local result = {}
  for k,v in pairs(default_values) do 
    result[k] = (custom_values[k] ~= nil) and custom_values[k] or v
  end
  return result
end

--- mutate first table, updating values from second table.
--- new keys from second table will be added to first.
--- nils in updated_values are ignored
-- @tparam table table_to_mutate table to mutate
-- @tparam table updated_values override values (take precedence)
-- @treturn table composite table
function tab.update(table_to_mutate, updated_values)
  for k,v in pairs(updated_values) do 
    if updated_values[k] ~= nil then
      table_to_mutate[k] = updated_values[k]
    end
  end
  return table_to_mutate
end

--- Create a new table with all values that pass the test implemented by the provided function.
-- @tparam table tbl table to check
-- @param condition callback function that tests all values of provided table, passes value and key as arguments
-- @treturn table table with values that pass the test
tab.select_values = function(tbl, condition)
  local t = {}

  for k,v in pairs(tbl) do
    if condition(v,k) then t[k] = v end
  end

  return t
end

return tab
