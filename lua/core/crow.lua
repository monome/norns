--- Crow Module
-- @module crow

local ii_events = require 'core/crow/ii_events'
local ii_actions = require 'core/crow/ii_actions'


-- system setup

_norns.crow = {}

_norns.crow.add = function(id, name, dev)
  norns.crow.dev = dev
  crow.add(id, name, dev)

  --- enable clock-in if needed
  if params.lookup["clock_source"] then
    if params:string("clock_source") == "crow" then
      crow.input[1].change = function() end
      crow.input[1].mode("change",2,0.1,"rising")
    end
  end
end

_norns.crow.remove = function(id)
  norns.crow.dev = nil
  crow.remove(id)
end

_norns.crow.event = function(id, line)
  line = string.sub(line,1,-2) -- strip newline
  if string.find(line,"^%^%^") then
    line = line:gsub("%^%^","norns.crow.")
    assert(load(line))()
  else
    crow.receive(line)
  end
end


-- system event callbacks

norns.crow = {}

function norns.crow.identity(...) print("crow identity: " .. ...) end
function norns.crow.version(...) print("crow version: " .. ...) end
function norns.crow.stream(n,v) crow.input[n].stream(v) end
function norns.crow.change(n,v) crow.input[n].change(v) end
function norns.crow.output(n,v) crow.output[n].receive(v) end
function norns.crow.done(n) crow.output[n].done() end
function norns.crow.running(n,v) crow.output[n].running(v) end
function norns.crow.midi(...) crow.midi(...) end

norns.crow.ii = ii_events


-- userspace functions

local crow = {}

--- send version
function crow.version() crow.send("^^v") end
--- send identity
function crow.identity() crow.send("^^i") end
--- reset crow
function crow.reset() crow.send("crow.reset()") end
--- send kill
function crow.kill() crow.send("^^k") end
--- send clear
function crow.clear() crow.send("^^c") end
--- send a command
function crow.send(cmd)
  if norns.crow.dev then
    _norns.crow_send(norns.crow.dev,cmd)
  end
end
--- check if crow is connected
function crow.connected()
  return _norns.crow.dev ~= nil
end


-- crowlib aliases & metatable syntax enhancements

local input = {}

function input.new(x)
  local i = { n = x }
  i.query = function() crow.send("get_cv("..i.n..")") end
  i.stream = function(v) print("crow input stream: "..i.n.." "..v) end
  i.change = function(v) print("crow input change: "..i.n.." "..v) end
  i.mode = function(m,a,b,c)
    local cmd = string.format("input[%i].mode(%q",i.n,m)
    if a ~= nil then cmd = cmd .. "," .. a end
    if b ~= nil then cmd = cmd .. "," .. b end
    if c ~= nil then cmd = string.format("%s,%q",cmd,c) end
    cmd = cmd .. ")"
    crow.send(cmd)
  end
  setmetatable(i,input)
  return i
end


local output = {}

function output.new(x)
  local o = { n = x }
  o._volts = 0
  o._shape = 'linear'
  o._slew = 0
  o.query = function() crow.send("get_out("..o.n..")") end
  o.receive = function(v) print("crow output receive: "..o.n.." "..v) end
  o.done = function() print("crow output action done: "..o.n) end
  o.running = function(v) print("crow output is running?: "..o.n.." "..v) end
  -- WILL BE DEPRECATED in 2.3.0
  o.execute = function() crow.send("output["..o.n.."]()") end
  setmetatable(o,output)
  return o
end

local function action_string(v)
  if type(v) ~= 'table' then
    return v
  end  
  local arg_string = ''
  for _,arg in ipairs(v) do
    if arg then
      if string.len(arg_string) ~= 0 then
        arg_string = arg_string..', '
      end
      arg_string = arg_string..arg
    end
  end
  return '{'..arg_string..'}'
end

output.__newindex = function(self, i, v)
  local me = "output["..self.n.."]"
  if i == 'volts' then
    self._volts = v
    crow.send(me..".volts="..v)
  elseif i == 'shape' then
    self._shape = v
    crow.send(string.format("%s.shape=%q",me,v))
  elseif i == 'slew' then
    self._slew = v
    crow.send(me..".slew="..v)
  elseif i == 'action' then
    crow.send(me..".action = "..action_string(v))
  elseif i == 'done' then
    crow.send(me..".done=_c.tell('done',"..me..".channel)")
  end
end

output.__index = function(self, i)
  if i == 'volts' then
    return self._volts
  elseif i == 'shape' then
    return self._shape
  elseif i == 'slew' then
    return self._slew
  elseif i == 'running' then
    local me = "output["..self.n.."]"
    crow.send("_c.tell('running',"..me..".channel,"..me..".running)")
  end
end

output.__call = function(self,arg)
  local args = ""
  if type(arg) == "string" then -- asl directive
    if arg == "start"
    or arg == "restart"
    or arg == "attack"
    or arg == "release"
    or arg == "step"
    or arg == "unlock" then
      args = string.format("%q",arg)
    else -- boolean or asl
      args = arg
    end
  end
  crow.send("output["..self.n.."]("..args..")")
end

setmetatable(output, output)


crow.cal = {}
crow.cal.test = function() crow.send("cal.test()") end
crow.cal.default = function() crow.send("cal.default()") end
crow.cal.print = function() crow.send("cal.print()") end


crow.ii = ii_actions
crow.ii.help = function() crow.send("ii.help()") end
crow.ii.pullup = function(x)
  local truth = (x == true or x == 1) and 'true' or 'false'
  crow.send("ii.pullup("..truth..")")
end


-- initialize

crow.init = function()
  --- return crow to blank slate when loading a new norns script
  crow.reset()

  --- customizable system events
  crow.add = function(id, name, dev)
    crow.reset() -- reset crow env on (re)connection
    print(">>>>>> norns.crow.add / " .. id .. " / " .. name)
  end
  crow.remove = function(id) print(">>>>>> norns.crow.remove " .. id) end
  crow.receive = function(...) print("crow:",...) end

  --- data structures & default callbacks
  crow.input = { input.new(1), input.new(2) }
  crow.output = { output.new(1), output.new(2), output.new(3), output.new(4) }
  crow.midi = function(...) print("crow midi:",...) end
  crow.ii.init()
end

crow.init() -- ensure data structures exist for metamethods


return crow
