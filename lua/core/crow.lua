--- Crow Module
-- @module crow


--- Quote sub-library
-- fns for stringifying data-structures
-- output is a string that can be read back as lua code with call()

local OPTIMIZE_LENGTH = false -- true reduces tx-length of strings, but takes longer to calc

local function quotekey(ix)
    -- stringify a table key with explicit [] style
    local fstr = (type(ix)=='number') and '[%g]' or '[%q]'
    return string.format(fstr, ix)
end

local function quote(val, ...)
    -- stringify any data so lua can load() it
    if ... ~= nil then
        local t = {quote(val)} -- capture 1st arg
        for _,v in ipairs{...} do -- capture varargs
            table.insert(t, quote(v))
        end
        return table.concat(t, ',')
    end
    if type(val) == 'string' then return string.format('%q',val)
    elseif type(val) ~= 'table' then return tostring(val)
    else -- recur per table element
        local t = {}
        if OPTIMIZE_LENGTH then
            local max = 0
            -- add array-style keys for reduced string length
            for k,v in ipairs(val) do
                table.insert(t, quote(v))
                max = k -- save highest ipair key
            end
            for k,v in pairs(val) do
                -- match on any key that wasn't caught by ipairs (without needing to copy the table)
                    -- not a number, is a float, is a sparse int key, is a zero or less int key
                if type(k) ~= 'number' or k ~= math.floor(k) or k > max or k < 1 then
                    table.insert(t, quotekey(k) .. '=' .. quote(v))
                end
            end
        else -- faster to build, but transmission is longer
            for k,v in pairs(val) do
                table.insert(t, quotekey(k) .. '=' .. quote(v))
            end
        end
        return string.format('{%s}', table.concat(t, ','))
    end
end



--- system level configuration
_norns.crow = {}

_norns.crow.add = function(id, name, dev)
  norns.crow.dev = dev
  norns.crow.add(id, name, dev)

  --- enable clock-in if needed
  if params.lookup["clock_source"] then
    if params:string("clock_source") == "crow" then
      norns.crow.clock_enable()
    end
  end
end

_norns.crow.remove = function(id)
  norns.crow.dev = nil
  norns.crow.remove(id)
end

local ebuffer = ""
_norns.crow.event = function(id, line)
  local function evalcrow(line)
    line, reps = (ebuffer..line):gsub("%^%^", "norns.crow.events.")
    ebuffer = "" -- ebuffer has been processed, so reset it
    if reps > 0 then
      assert(load(line))()
    else
      crow.receive(line)
    end
  end

  local n, m = line:find "%c+"
  if not n then -- no control chars found
    ebuffer = ebuffer .. line -- line incomplete so add it to the buffer
  else
    evalcrow(line:sub(1, n-1)) -- process complete segment
    if m < #line then -- recur with post-whitespace remainder
      _norns.crow.event(id, line:sub(m+1, -1))
    end
  end
end



--- for talking *about* crow
-- contain support / functions for ^^ events from crow
norns.crow = {}

-- communication i/o
norns.crow.send = function(cmd)
  if norns.crow.dev then
    _norns.crow_send(norns.crow.dev, cmd)
  end
end

function norns.crow.connected()
    return norns.crow.dev ~= nil
end

function norns.crow.loadscript(file, is_persistent, cont)
  -- run / upload userscript
  local abspath = norns.state.path .. 'crow/' .. file
  if not util.file_exists(abspath) then
    abspath = _path.code .. file
    if not util.file_exists(abspath) then
      print("norns.crow.loadscript: can't find file "..file)
      return
    end
  end

  local function upload(file, is_persistent, cont)
    -- TODO refine these clock.sleep(). can likely be reduced.
    norns.crow.send("^^s")
    clock.sleep(0.2)
    for line in io.lines(file) do
      norns.crow.send(line)
      clock.sleep(0.01)
    end
    clock.sleep(0.2)
    norns.crow.send(is_persistent and "^^w" or "^^e")
    if cont then
      clock.sleep(is_persistent and 0.5 or 0.2) -- ensure flash is complete
      cont() -- call continuation function
    end
  end

  print("crow loading: ".. file)
  clock.run(upload, abspath, is_persistent, cont)
end


-- event handling
function norns.crow.register_event(fn)
  local n = #norns.crow.events + 1
  local c = ""
  if     n <= 26 then c = string.char(n + 64) -- uppercase alphas
  elseif n <= 52 then c = string.char(n + 70) -- lowercase alphas
  else print("ERROR: can't register event. out of indices. only 52 handlers allowed.")
  end
  norns.crow.events[c] = fn -- store the function in the event table
  return c -- return the key reference for linking to crow event
end

function norns.crow.reset_events()
  -- configure the list of static crow events
  norns.crow.events = setmetatable({
    identity = function(...) print("crow identity: " .. ...) end,
    version  = function(...) print("crow version: " .. ...) end,
  },{
    __index = function(self, ix)
      return function(...) print("unused event: ^^"..ix.."(".. quote(...) ..")") end
    end,
  })
end


-- reset state 
function norns.crow.init()
  -- clear crow's VM allowing for full customization by norns script
  crow.reset()

  -- clear all but the static crow events
  norns.crow.reset_events()

  -- customizable system events
  norns.crow.add = function(id, name, dev)
    crow.reset() -- reset crow env on (re)connection
    print(">>>>>> norns.crow.add / " .. id .. " / " .. name)
  end
  norns.crow.remove = function(id) print(">>>>>> norns.crow.remove " .. id) end
  norns.crow.receive = function(...) print("crow:",...) end
end


--- helper functions for common crow actions from norns
norns.crow.clock_enable = function()
-- TODO confirm that the new system denecessitates reset_events as we dynamically reassign the fn
  norns.crow.send("input[1]:reset_events()") -- ensure crow's callback has not been redefined
  crow.input[1].change = function() end
  crow.input[1].mode("change",2,0.1,"rising")
end



--- crow namespace support
-- for talking *to* crow
crow = {}

-- fns with custom syntax
function crow.version()  crow "^^v" end
function crow.identity() crow "^^i" end
function crow.reset()    crow "crow.reset()" end
function crow.kill()     crow "^^k" end
function crow.clear()    crow "^^c" end
function crow.send(...)  crow(...) end -- alias for legacy scripts
-- crow.connected -> norns.crow.connected
-- crow.add       -> norns.crow.add
-- crow.remove    -> norns.crow.remove
-- crow.receive   -> norns.crow.receive
-- crow.init      -> norns.crow.init


--- dynamic namespace support
-- enables full syntax support for calling crow functions, and setting crow values
-- does *not* support directly querying values or function responses from crow
-- when setting crow namespace fns they create stubs on crow that forward to the norns events
local crowSub = {
    __index = function(self, ix)
        -- build up the table access key chain
        self.str = self.str .. quotekey(ix)
        return self
    end,

    __newindex = function(self, ix, val)
        -- set a table value on crow
        sval = ""
        if type(val) == 'function' then
            -- assigning a function to a crow variable, causes crow to forward that fn call to norns
            local n = norns.crow.register_event(val) -- register the event & get a dynamic key
            sval = 'function(...)_c.tell('..quote(n)..',...)end')
        else
            sval = quote(val)
        end
        norns.crow.send(self.str .. quotekey(ix) .. '=' .. sval)
    end,

    __call = function(self, ...)
        local qt = quote(...)
        norns.crow.send(self.str .. '(' .. qt .. ')')
    end,
}

-- set global var on crow
crow.__newindex = function(self, ix, val) norns.crow.send(ix .. '=' .. quote(val)) end

-- create a table that will resolve to a crow.send call in crowSub
crow.__index = function(self, ix) return setmetatable({str=ix}, crowSub) end

-- call crow with a literal string to execute that string on crow:
-- crow "output[1].volts = 3"
crow.__call = function(self, ...) norns.crow.send(...) end

setmetatable(crow, crow)



norns.crow.init() -- ensure data structures exist for metamethods

return crow
