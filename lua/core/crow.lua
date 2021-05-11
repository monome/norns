
-- _norns = {crow_send = function(...) end} -- REMOVE THIS. TESTING ONLY
-- norns = {} -- REMOVE THIS. TESTING ONLY

--- Crow Module
-- @module crow


--- helper fns for quoting (aka stringifying) lua constructs
local function quotekey(ix)
    -- stringify table keys with [] style
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
        for k,v in pairs(val) do
            table.insert(t, quotekey(k) .. '=' .. quote(v))
        end
        return string.format('{%s}', table.concat(t, ','))
    end
end


-- system setup

_norns.crow = {}

_norns.crow.add = function(id, name, dev)
  norns.crow.dev = dev
  crow.add(id, name, dev)

  --- enable clock-in if needed
  if params.lookup["clock_source"] then
    if params:string("clock_source") == "crow" then
      crow.send("input[1]:reset_events()") -- ensure crow's callback has not been redefined
      crow.input[1].change = function() end
      crow.input[1].mode("change",2,0.1,"rising")
    end
  end
end

_norns.crow.remove = function(id)
  norns.crow.dev = nil
  crow.remove(id)
end

local ebuffer = ""
_norns.crow.event = function(id, line)
  local function evalcrow(line)
    line, reps = (ebuffer..line):gsub("%^%^","norns.crow.") -- ebuffer contains the earlier message
    ebuffer = "" -- cleared so ready to process
    if reps > 0 then
      assert(load(line))()
    else
      crow.receive(line)
    end
  end

  local n, m = string.find(line,"%c+")
  if not n then -- no control chars found
    ebuffer = ebuffer .. line -- line incomplete so add it to the buffer
  else
    if m < #line then -- split & recur
      evalcrow(string.sub(line, 1, n-1)) -- complete
      _norns.crow.event(id, string.sub(line, m+1, -1))
    else -- string ends with control
      evalcrow(string.sub(line, 1, n-1))
    end
  end
end


-- system event callbacks

norns.crow = {}

function norns.crow.identity(...) print("crow identity: " .. ...) end
function norns.crow.version(...) print("crow version: " .. ...) end
function norns.crow.new_event()
    if not norns.crow.event then norns.crow.event = {} end -- create table if doesn't exist
    local n = #norns.crow.events + 1
    if     n <= 26 then return string.char(n + 64) -- uppercase alphas
    elseif n <= 52 then return string.char(n + 70) -- lowercase alphas
    else print('ERROR: norns.crow.events out of indices. only 52 handlers allowed.')
    end
end
setmetatable(norns.crow,{
    __index = function(self, ix)
        if norns.crow.events[ix] then
            return norns.crow.events[ix]
        elseif ix ~= "dev" then
	    -- FIXME are there other elements than 'dev' in norns.crow used by the system?
            return function(...) print("unused event: ^^"..ix.."(".. quote(...) ..")") end
        end
    end
})


--- crow table
-- defines fns that relate to crow but run on norns
-- defines ^^response event handlers
-- anything else is forwarded to crow via metamethods
crow = {}

-- explicit fns with custom handling
function crow.version() crow.send "^^v" end
function crow.identity() crow.send "^^i" end
function crow.reset() print'RESET';crow.send "crow.reset()"; print'AFTER' end
function crow.kill() crow.send "^^k" end
function crow.clear() crow.send "^^c" end

function crow.connected()
    return norns.crow.dev ~= nil
end

crow.send = function(cmd)
    if norns.crow.dev then
        _norns.crow_send(norns.crow.dev, cmd)
    end
end

--- run / upload userscript
function crow.loadscript(file, is_persistent)
  local abspath = norns.state.path .. 'crow/' .. file
  if not util.file_exists(abspath) then
    abspath = _path.code .. file
    if not util.file_exists(abspath) then
      print("crow.loadscript: can't find file "..file)
      return
    end
  end

  local function upload(file, is_persistent)
    -- TODO refine these clock.sleep(). can likely be reduced.
    crow.send("^^s")
    clock.sleep(0.2)
    for line in io.lines(file) do
      crow.send(line)
      clock.sleep(0.01)
    end
    clock.sleep(0.2)
    crow.send(is_persistent and "^^w" or "^^e")
  end

  print("crow loading: ".. file)
  clock.run(upload, abspath, is_persistent)
end


crow.init = function()
	print'CROW INITTTTT'
  -- reset dynamic event handler list
  norns.crow.events = {}

  -- return crow to blank slate when loading a new norns script
  crow.reset()

  -- customizable system events
  crow.add = function(id, name, dev)
    crow.reset() -- reset crow env on (re)connection
    print(">>>>>> norns.crow.add / " .. id .. " / " .. name)
  end
  crow.remove = function(id) print(">>>>>> norns.crow.remove " .. id) end
  crow.receive = function(...) print("crow:",...) end
end

crow.init() -- ensure data structures exist for metamethods


--- crow namespace support
-- enables full syntax support for calling crow functions, and setting crow values
-- does *not* support directly querying values or function responses from crow
-- when setting crow namespace fns they create stubs on crow that forward to the norns events

-- metamethods for building up strings of underlying call
-- and calling / setting values on crow
-- note: fn calls & value queries don't return anything
-- ASL constructs are supported by wrapping ASL descriptions in a string
crowSub = {
    __index = function(self, ix)
        -- build up the table access key chain
        self.str = self.str .. quotekey(ix)
        return self
    end,

    __newindex = function(self, ix, val)
        -- set a table value on crow
        if type(val) == 'function' then
            -- functions will install an async function on crow, forwarding to a dynamic norns handler
            local n = norns.crow.new_event()
            norns.crow.events[n] = val -- store the event handler function privately
            self.send(self.str .. quotekey(ix) .. '=function(...)_c.tell('..quote(n)..',...)end')
        else
            self.send(self.str .. quotekey(ix) .. '=' .. quote(val))
        end
    end,

    __call = function(self, ...)
        local qt = quote(...)
        self.send(self.str .. '(' .. qt .. ')')
    end,
}

-- set global var on crow
crow.__newindex = function(self, ix, val) crow.send(ix .. '=' .. quote(val)) end

-- create a table that will resolve to a crow.send call in crowSub
crow.__index = function(self, ix) return setmetatable({send=self.send, str=ix}, crowSub) end

setmetatable(crow, crow)

return crow -- comment out to run tests

--[[




--- TEST code

-- custom crow.send fn for testing locally
tx = '' -- write the output val to tx for test assertion
crow.send = function(...) tx = ... end


function test(_, ...) -- first arg is just a nop placeholder for the fn under test
    local strs = {...}
    for k,v in pairs(strs) do
        if tx == v then return end
    end
    print('test failed')
    print('  expected: '..strs[1])
    print('  sent:     '..tx)
end

-- call all the fns & setters of the crow global space
test( crow.reset() --> this is a predefined fn, doesn't use the metamethods
    ,'crow.reset()')

test( crow.cal.test()
    ,'cal["test"]()')

test( crow.ii.pullup(true)
    , 'ii["pullup"](true)')

test( crow.input[1].mode('stream', 0.01)
    , 'input[1]["mode"]("stream",0.01)')

test( crow.input[1]{ mode = 'change', direction = 'rising' }
    , 'input[1]({["mode"]="change",["direction"]="rising"})'
    , 'input[1]({["direction"]="rising",["mode"]="change"})') -- table order is undefined

test( crow.ii.jf.mode(1)
    , 'ii["jf"]["mode"](1)')

test( crow.ii.kria.get 'preset'
    , 'ii["kria"]["get"]("preset")')

test( crow.output[1]()
    , 'output[1]()')

test( crow.output[1].query()
    , 'output[1]["query"]()')


-- assignments can't be in fn calls, so we just chain the test fn
crow.output[2].volts = 3.3
test(_, 'output[2]["volts"]=3.3')

crow.output[3].slew = 0.001
test(_, 'output[3]["slew"]=0.001')

crow.output[4].shape = 'expo'
test(_, 'output[4]["shape"]="expo"')

crow.output[1].scale = {0,2,7,5}
test(_, 'output[1]["scale"]={[1]=0,[2]=2,[3]=7,[4]=5}')

-- FIXME
-- FAILING due to wrapping string value in quotes, but we want to unwrap
-- this could just be handled on crow, to load() the string?
-- crow.output[1].action = "{ to(0,0), to(5,0.1), to(1,2) }"
-- test(_, 'output[1]["action"]={ to(0,0), to(5,0.1), to(1,2) }')


-- event handlers
crow.output[1].receive = function(v) print('o[1].rx: '..v) end
test(_, 'output[1]["receive"]=function(...)_c.tell("A",...)end')

-- receive: ^^A(3.23)
-- _norns.crow.event captures & forwards to:function norns.crow.stream(n,v) crow.input[n].stream(v) end
norns.crow.A(3.23)


--[[
-- set global crow var
crow.myvar = 1

-- set table var on crow with string index
crow.tab.key = 2

-- set nested table var on crow with numeric index
crow.tab[3].key = 4

-- call global crow fn no arguments
crow.fn()

-- call global crow fn with arguments
crow.fn(6)

-- call table fn
crow.tab.fn(8,'string',9)

-- call table fn with table argument
crow.tab.fn{1,2,3,'string',3.2,-12389.00}

-- call table fn with k,v pair args
crow.tab.fn{a=1, b=2, c=3}
]]



--- norns syntax that needs added support on crow
-- output[n].execute() --> old syntax for: output[n]()
-- output[n].query() --> returns into: output[n].receive(v)


-- input[n].query() --> triggers input[n].stream callback
-- ]]
