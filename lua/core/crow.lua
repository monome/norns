local util = require 'util'

local function tostringwithquotes(s)
  return "'"..tostring(s).."'"
end

function _norns.crow_stream(n,v)
  crow.input[n].stream(v)
end

function _norns.crow_change(n,v)
  crow.input[n].change(v)
end

function _norns.crow_output(i,v)
  crow.output[i].receive(v)
end

function _norns.crow_identity(...)
  print("crow identity: " .. ...)
end

function _norns.crow_version(...)
  print("crow version: " .. ...)
end


norns.crow = {}

norns.crow.add = function(id, name, dev)
  print(">>>>>> norns.crow.add / " .. id .. " / " .. name)
  norns.crow.dev = dev
end

norns.crow.remove = function(id)
  print(">>>>>> norns.crow.remove " .. id)
  norns.crow.dev = nil
end

norns.crow.event = function(id, line)
  line = string.sub(line,1,-2) -- strip newline
  if util.string_starts(line,"^^") == true then
    line = line:gsub("%^^","_norns.crow_") 
    assert(load(line))()
    --print(line)
  else
    print("crow receive: "..line)
  end
end


-- ----

local input = {}

function input.new(x)
  local i = { n = x }
  i.query = function() crow.send("get_cv("..i.n..")") end
  i.stream = function(v) print("crow input stream: "..i.n.." "..v) end
  i.change = function(v) print("crow input change: "..i.n.." "..v) end
  i.mode = function(m) crow.send("input["..i.n.."].mode("..tostringwithquotes(m)..")") end
  setmetatable(i,input)
  return i
end

setmetatable(input, input)


local output = {}

function output.new(x)
  local o = { n = x }
  o.query = function() crow.send("get_out("..o.n..")") end
  o.receive = function(v) print("crow output receive: "..o.n.." "..v) end
  setmetatable(o,output)
  return o
end

output.__newindex = function(self, i, v)
  if i == 'volts' then
    crow.send("output["..self.n.."].volts="..v)
  end
end

setmetatable(output, output)

-- ----

local crow = {}

crow.output = { output.new(1), output.new(2) }
crow.input = { input.new(1), input.new(2), input.new(3), input.new(4)  }

function crow.version()
  crow.send("^^v")
end

function crow.identity()
  crow.send("^^i")
end

function crow.send(cmd)
  if norns.crow.dev then
    --print("crow send: "..cmd)
    _norns.crow_send(norns.crow.dev,cmd)
  end
end

crow.II = {}
crow.II.pullup = function(x) crow.send("II.pullup("..x..")") end
crow.II.jf = {}
crow.II.jf.trigger = function(x,y) crow.send("II.jf.trigger("..x..","..y..")") end




return crow
