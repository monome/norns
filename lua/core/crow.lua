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

function _norns.crow_midi(...)
  crow.input[n].midi(...)
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
  i.midi = function(...) print("crow midi: "..i.n.." ".. ...) end
  i.mode = function(m,v)
    local cmd = "input["..i.n.."].mode("..tostringwithquotes(m) 
    if v ~= nil then cmd = cmd .. "," .. v .. ")"
    else cmd = cmd .. ")" end
    crow.send(cmd)
  end
  setmetatable(i,input)
  return i
end

setmetatable(input, input)


local output = {}

function output.new(x)
  local o = { n = x }
  o.query = function() crow.send("get_out("..o.n..")") end
  o.receive = function(v) print("crow output receive: "..o.n.." "..v) end
  o.execute = function() crow.send("output["..o.n.."]()") end
  setmetatable(o,output)
  return o
end

output.__newindex = function(self, i, v)
  if i == 'volts' then
    self._volts = v
    crow.send("output["..self.n.."].volts="..v)
  elseif i == 'slew' then
    self._slew = v
    crow.send("output["..self.n.."].slew="..v)
  elseif i == 'action' then
    crow.send("output["..self.n.."].action = "..v)
  end
end

output.__index = function(self, i)
  if i == 'volts' then
    return self._volts
  elseif i == 'slew' then
    return self._slew
  end
end


setmetatable(output, output)

-- ----

local crow = {}

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

crow.input = { input.new(1), input.new(2) }
crow.output = { output.new(1), output.new(2), output.new(3), output.new(4) }

crow.init = function()
  crow.input = { input.new(1), input.new(2) }
  crow.input[1].mode("none")
  crow.input[2].mode("none")
  crow.output = { output.new(1), output.new(2), output.new(3), output.new(4) }
end


crow.II = {}
crow.II.pullup = function(x) if x == true then crow.send("II.pullup(true)")
  else crow.send("II.pullup(false)") end end
crow.II.jf = {}
crow.II.jf.trigger = function(ch,state) crow.send("II.jf.trigger("..ch..","..state..")") end
crow.II.jf.run_mode = function(mode) crow.send("II.jf.run_mode("..mode..")") end
crow.II.jf.run = function(volts) crow.send("II.jf.run("..volts..")") end
crow.II.jf.transpose = function(pitch) crow.send("II.jf.transpose("..pitch..")") end
crow.II.jf.vtrigger = function(ch,level) crow.send("II.jf.vtrigger("..ch..","..level..")") end
crow.II.jf.retune = function(ch,numerator,denominator) crow.send("II.jf.retune("..ch..","..numerator..","..denominator..")") end
crow.II.jf.mode = function(mode) crow.send("II.jf.mode("..mode..")") end
crow.II.jf.play_voice = function(channel,pitch,level) crow.send("II.jf.play_voice("..channel..","..pitch..","..level..")") end
crow.II.jf.play_note = function(pitch,level) crow.send("II.jf.play_note("..(pitch)..","..(level)..")") end
crow.II.jf.god_mode = function(state) crow.send("II.jf.god_mode("..state..")") end
crow.II.jf.tick = function(clock) crow.send("II.jf.tick("..clock..")") end
crow.II.jf.quantize = function(divisions) crow.send("II.jf.quantize("..divisions..")") end


return crow
