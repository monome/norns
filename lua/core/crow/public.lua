--- temporary lib file
-- extends norns.crow table with public discovery system


local Public = {}

function Public.clear()
  Public._names = {}
  Public._params = {}

end

Public.clear()

-- user callbacks
function Public.ready() print 'crow.public synchronized.' end
function Public.change(k,v) end


-- FIXME crow should broadcast a message after it has loaded a script & executed init()
function Public.loadscript(file, is_persistent)
  -- start upload
  co = crow.loadscript(file, is_persistent)
  clock.run( function(co, p)
      -- wait for loadscript to complete
      clock.sleep(0.5) -- minimum time for upload to complete
      if coroutine.status(co) ~= 'dead' then clock.sleep(0.1) end -- sleep until completion
      if p then clock.sleep(0.2) end -- extra time for flash write
      clock.sleep(1.5) -- wait for lua env to be ready. TODO use a callback on end-of-init
      -- reset the public storage
      Public.clear()
      -- request params from crow
      crow.send "public.discover()"
    end, clock.threads[co], p) -- we grab the coroutine itself, not clock index
end


-- from crow: ^^pub(name,val,{type})
function Public.add(name, val, typ)
  if name == '_end' then
    Public.ready()
  else
    -- add name to dictionary with linked index
    local ix = Public._names[name] -- look for existing declaration
    if not ix then -- new addition
      ix = #(Public._params) + 1
      Public._names[name] = ix
    end
    Public._params[ix] = { name = name, val = val }
    local p = Public._params[ix]
    print("adding: " .. name .. "=" .. tostring(val))
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
          else -- capture enum
            p.type = 'enum'
            p.enum = {}
            for i=2,len do
              p.enum[i-1] = typ[i]
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
  elseif p.type == 'enum' then
    tmp = p.enum[1] -- default to first elem
    for k,v in ipairs(p.enum) do
      if v == p.val then
        tmp = p.enum[util.wrap(k+z, 1, #p.enum)] -- increment index, return name
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
