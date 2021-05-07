--- temporary lib file
-- extends norns.crow table with public discovery system
--
local function quotekey(ix)
  -- stringify table keys with [] style
  local fstr = (type(ix)=='number') and '[%g]' or '[%q]'
  return string.format(fstr, ix)
end

local function quote(value)
  -- stringify anything so it can be read as lua code
  if type(value) == 'string' then return string.format('%q',value)
  elseif type(value) ~= 'table' then return tostring(value)
  else -- recur per table element
    local t = {}
    for k,v in pairs(value) do
      table.insert(t, quotekey(k) .. '=' .. quote(v))
    end
    return string.format('{%s}', table.concat(t, ','))
  end
end


local Public = {}

function Public.clear()
  Public._names = {}
  Public._params = {}

  local vmt = {
    __newindex = function(self, ix, val)
      rawset(self,'_'..ix,val)
      Public.change() -- no args bc it doesn't fit the model
    end,
    __index = function(self, ix)
      return rawget(self, '_'..ix)
    end
  }
  Public.viewing = { -- clear viewed vals on load. metatable causes Public.change() trigger
    input = setmetatable({},vmt),
    output = setmetatable({},vmt),
  }
end

Public.clear()

-- user callbacks
function Public.change(k,v) end
function Public.discovered() print'crow.public discovered' end

-- called upon crow.init() completion
function Public.ready()
  -- reset the public storage
  Public.clear()
  -- request params from crow
  crow.send "public.discover()"
end

-- from crow: ^^pub(name,val,{type})
function Public.add(name, val, typ)
  if name == '_end' then
    Public.discovered()
  else
    -- add name to dictionary with linked index
    local ix = Public._names[name] -- look for existing declaration
    if not ix then -- new addition
      ix = #(Public._params) + 1
      Public._names[name] = ix
    end
    Public._params[ix] = { name = name, val = val }
    local p = Public._params[ix]
    -- print("adding: " .. name .. "=" .. tostring(val))
    if type(val) == 'table' then
      p.list = true
      p.listix = 1
      p.listlen = #val
    else p.list = false end
    if typ then
      local len = #typ
      if len > 0 then
        if type(typ[1]) == 'string' then
          if len == 1 then
            p.type = typ[1]
          else -- capture option
            p.type = 'option'
            p.option = {}
            for i=2,len do
              p.option[i-1] = typ[i]
            end
          end
        elseif len == 2 then
          p.min = typ[1]
          p.max = typ[2]
          p.range = typ[2]-typ[1]
        elseif len == 3 then
          p.min = typ[1]
          p.max = typ[2]
          p.range = typ[2]-typ[1]
          p.type = typ[3]
        end
      end
    end
  end
end


function Public.get_count()
  return #Public._params
end

function Public.get_index(ix)
  return Public._params[ix]
end

-- delta expects integer steps (eg from enc())
-- alt is for secondary param (eg element selection for lists)
-- FIXME needs refactor to avoid early return on lists
function Public.delta(ix, z, alt)
  local p = Public._params[ix]
  local tmp = 0
  if p.list then
    if alt then
      p.listix = util.wrap(p.listix + z, 1, p.listlen)
      return -- FIXME refactor to avoid this
    else -- TODO add type support for min/max, floats & scaling
      p.val[p.listix] = p.val[p.listix] + z
      tmp = p.val -- re-write the table to cause underlying metamethod to transmit change
    end
  elseif p.type == 'option' then
    tmp = p.option[1] -- default to first elem
    for k,v in ipairs(p.option) do
      if v == p.val then
        tmp = p.option[util.wrap(k+z, 1, #p.option)] -- increment index, return name
        break
      end
    end
  else -- numeric
    if p.type == 'integer' then
      -- do nothing
    else -- assume number
      -- scale z to 100 increments, or 0.01 steps if no range
      if p.range then z = z * (p.range / 100)
      else z = z / 100 end
    end
    tmp = p.val + z
  end
  Public[p.name] = tmp -- use metamethod to cause remote update & clamp
end

function Public.update(name, val, sub)
  local kix = Public._names[name]
  if kix then
    local p = Public._params[kix]
    if sub then
      p.val[sub] = val
    else
      p.val = val
    end
    Public.change(name,p.val) -- user callback (for redrawing display)
  end
end

function Public.quoteparams()
  local t = {}
  for k,v in pairs(Public._names) do
    local val = quote(Public._params[v].val)
    if val ~= nil then -- TEMP protect against tables
      table.insert(t, 'public.'..k..'='..val)
    end
  end
  return table.concat(t,'\n')
end

function Public.freezescript(path)
  local abspath = crow.findscript(path)
  if not abspath then
    print("crow.freezescript: can't find file "..path)
    return
  end

  local script = assert(io.open(abspath, "r"))
  local tmp = assert(io.open(_path.code .. "_crowtmp.lua", "wb")) -- create file in code so it's found
  
  tmp:write( script:read("*all")) -- copy script into tmp
  script:close()

  tmp:write(Public.quoteparams())

  tmp:close()

  -- TODO should delete tmpfile after use. can add a continuation fn to loadscript
  crow.loadscript("_crowtmp.lua", true)
end


-- TODO deprecate with auto-namespace handling
Public.view = {input={}, output={}}
function Public.view.all(s)
  if s==nil then s = 1 end
  crow.send("public.view.all(".. s ..")")
end
for n=1,2 do Public.view.input[n] = function(s)
    if s==nil then s = 1 end
    crow.send("public.view.input["..n.."](".. s ..")")
  end
end
for n=1,4 do Public.view.output[n] = function(s)
    if s==nil then s = 1 end
    crow.send("public.view.output["..n.."](".. s ..")")
  end
end


--- METAMETHODS
-- setters
Public.__newindex = function(self, ix, val)
  local kix = Public._names[ix]
  if kix then
    -- TODO apply type limits (but not scaling)
    local p = Public._params[kix]
    local vstring = ""
    if p.list then
      p.val = val -- update internal table
      vstring = "{"
      for k,v in ipairs(p.val) do
        vstring = vstring .. tostring(v) .. ","
      end
      vstring = vstring .. "}"
    else -- not a list
      if p.range then
        val = util.clamp(val, p.min, p.max)
      end
      p.val = val -- update internal representation
      vstring = tostring(p.val) -- stringify for sending to crow
    end
    crow.send("public.update('"..p.name.."',"..vstring..")")
  end
end

-- getters
Public.__index = function(self, ix)
  local kix = Public._names.ix
  if kix then
    return Params._params[kix].val
  end
end


setmetatable(Public,Public)

return Public
