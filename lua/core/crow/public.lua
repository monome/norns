--- public.lua

quote = require 'core/crow/quote'

local Public = {}

function Public.clear()
  Public._names = {}
  Public._params = {}

  -- clear viewed vals on load
  Public.viewing = {
    input = {},
    output = {},
  }

  Public.change() -- inform an active script that state has changed
end


function Public.reset()
  -- customizable user events
  Public.change = function(k,v) end
  Public.discovered = function() print'crow.public discovered' end

  Public.clear()
end


--- system events
-- all triggered by received ^^ events from crow
function Public.ready()
  -- called upon crow.init() completion
  Public.clear() -- reset the public storage -- FIXME interacts with public.view outside init()
  crow.public.discover()
  -- norns.crow.send "public.discover()" -- request params from crow
end

function Public.view(n,c,v)
  Public.viewing[n][c] = v
  Public.change() -- no args, as these are anonymous vars
end

function Public.add(name, val, typ)
  if name == '_end' then
    Public.discovered()
  elseif name == '_clear' then
    Public.clear() -- clears state & informs an active script
  else
    -- add name to dictionary with linked index
    local ix = Public._names[name] -- look for existing declaration
    if not ix then -- new addition
      ix = #(Public._params) + 1
      Public._names[name] = ix
    end
    Public._params[ix] = { name = name, val = val }
    local p = Public._params[ix]
    if type(val) == 'table' then
      p.list = true
      p.listix = 1
      p.listlen = #val
    else p.list = false end
    Public.capture_type(p, typ)
  end
end

function Public.update(name, val, sub)
  -- triggered on remote update from crow device
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


--- helper fns for system events
function Public.capture_type(p, typ)
  -- capture range/type-annotation into known form
  -- assumes typ is a table (can be empty)
  local len = #typ -- type is always sent, even if empty table
  if type(typ[1]) == 'string' then
    if len == 1 then -- single string is a type annotation
      p.type = typ[1]
    else -- more than 1 string is an option type
      p.type = 'option'
      p.option = {}
      for i=2,len do
        table.insert(p.option, typ[i])
      end
      return -- EARLY RETURN. option type is complete
    end
  else -- numeric / table types
    if len >= 2 then -- min/max
      p.min = typ[1]
      p.max = typ[2]
      p.range = typ[2]-typ[1]
    end
    p.type = typ[3] -- may be nil
  end
  if p.type and p.type:sub(1,1) == '@' then -- capture readonly char
    p.readonly = true
    p.type = p.type:sub(2) -- drop '@' after marking readonly
  end
  if not p.type or p.type:len() == 0 then -- no string: infer numeric type
    p.type = Public.inferint(p) and 'int' or 'float'
  else -- there is a string type
    if p.type == 'exp' then
      p.exp = true
      p.type = 'float'
    elseif p.type == 'xslider' then
      p.exp = true
      p.type = 'slider'
    end
  end
end

function Public.inferint(p)
  -- return true only if the public var appears to be an integer
  local function not_int(v)
    -- return true if the arg is not representable as an integer
    if v and v ~= math.floor(v) then return true end
  end
  if p.list then
    for i=1,p.listlen do
      if not_int(p.val[i]) then return end
    end
  else if not_int(p.val) then return end
  end
  if not_int(p.min) then return end
  if not_int(p.max) then return end
  return true
end



--- userspace fns
function Public.get_count() return #Public._params end
function Public.get_index(ix) return Public._params[ix] end
function Public.lookup(name)
  local ix = Public._names[name]
  if ix then return Public._params[ix] end
end

function Public.freezescript(path)
  local abspath = norns.crow.findscript(path)
  if not abspath then
    print("norns.crow.public.freezescript: can't find file "..path)
    return
  end

  local script = assert(io.open(abspath, "r"))
  local tmp = assert(io.open(_path.code .. "_crowtmp.lua", "wb")) -- create file in code so it's found
  
  tmp:write( script:read("*all")) -- copy script into tmp
  script:close()

  local function quoteparams()
    -- stringify current list of public params (for freeze)
    local t = {}
    for k,v in pairs(Public._names) do
      table.insert(t, 'public.'..k..'='..quote(Public._params[v].val))
    end
    return table.concat(t,'\n')
  end

  tmp:write(quoteparams()) -- append public params to end of copy
  tmp:close()

  -- TODO should delete tmpfile after use. can add a continuation fn to loadscript
  norns.crow.loadscript("_crowtmp.lua", true)
end


-- assumes integer steps (eg from enc())
-- alt is for secondary param (eg element selection for lists)
function Public.delta(ix, z, alt)
  -- increment a public variable with type awareness
  local p = Public._params[ix]
  local tmp = 0
  if p.readonly then return -- can't delta this param
  elseif p.list then
    if alt then -- update table index
      p.listix = util.wrap(p.listix + z, 1, p.listlen)
      return -- EARLY RETURN as we're updating the index not the value
    else
      p.val[p.listix] = Public.increment(p.val[p.listix], z, p)
      tmp = p.val -- re-write the table to cause underlying metamethod to transmit change
    end
  elseif p.type == 'option' then
    tmp = p.option[1] -- default to first elem
    -- TODO avoid this search by building a reverse-lookup for p.option
    for k,v in ipairs(p.option) do
      if v == p.val then -- iterate through table to find current ix
        tmp = p.option[util.wrap(k+z, 1, #p.option)] -- increment index, return name
        break
      end
    end
  else -- numeric
    tmp = Public.increment(p.val, z, p)
  end
  Public.io[p.name] = tmp -- use metamethod to cause remote update & clamp
end


--- helper fns
function Public.increment(val, z, p)
  if p.exp then
    z = z * math.abs(val)/100 -- step is proportional to value
  elseif p.type == 'float' or p.type == 'slider' then
    if p.range then z = z * (p.range / 100) -- 100 increments over range
    else z = z / 50 end -- 0.02 steps
  end
  return val + z
end



--- crow namespace functions
-- anything here is written as if talking to crow directly
Public.io = {
  view = setmetatable({},{
    -- folds crow.public.view.* back to the crow namespace
    __index = function(self, ix)
      return crow["public.view."..ix]
    end
  }),

  discover = function() norns.crow.send "public.discover()" end,
}

Public.io.__newindex = function(self, ix, val)
  local p = Public.lookup(ix)
  if p then
    if p.range then
      if p.list then -- clamp all values in a list
        for k,v in ipairs(val) do
          val[k] = util.clamp(v, p.min, p.max)
        end
      else -- clamp a single value
        val = util.clamp(val, p.min, p.max)
      end
    end
    p.val = val -- update internal representation
    norns.crow.send("public.update('"..p.name.."',"..quote(p.val)..")")
  else
    print("error: public."..ix.." must be declared in a crow script.")
  end
end

Public.io.__index = function(self, ix)
  local p = Public.lookup(ix)
  if p then return p.val end
end

setmetatable(Public.io,Public.io)

Public.reset() -- ensure all datastructres exist

return Public
