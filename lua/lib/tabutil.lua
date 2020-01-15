--- table utility
-- @module tabutil
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
end

--- Create a read-only proxy for a given table.
-- @param params params.table is the table to proxy, params.except a list of writable keys
-- @treturn table the proxied read-only table
function tab.readonly(params)
  local t = params.table
  local exceptions = params.except or {}
  local proxy = {}
  local mt = {
    __index = t,
    __newindex = function (_,k,v)
      if (tab.contains(exceptions, k)) then
        t[k] = v
      else
        error("'"..k.."', a read-only key, cannot be re-assigned.")
      end
    end,
    __pairs = function (_) return pairs(proxy) end,
    __ipairs = function (_) return ipairs(proxy) end,
  }
  setmetatable(proxy, mt)
  return proxy
end

return tab
