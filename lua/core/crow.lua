local util = require 'util'

local function tostringwithquotes(s)
  return "'"..tostring(s).."'"
end

norns.crow = {}
function norns.crow.identity(...) print("crow identity: " .. ...) end
function norns.crow.version(...) print("crow version: " .. ...) end
function norns.crow.stream(n,v) crow.input[n].stream(v) end
function norns.crow.change(n,v) crow.input[n].change(v) end
function norns.crow.output(i,v) crow.output[i].receive(v) end
function norns.crow.midi(...) crow.midi(...) end

norns.crow.ii = {}
function norns.crow.ii.ansible(i,v) crow.ii.ansible.event(i,v) end
function norns.crow.ii.kria(i,v) crow.ii.kria.event(i,v) end
function norns.crow.ii.meadowphysics(i,v) crow.ii.meadowphysics.event(i,v) end
function norns.crow.ii.wslash(i,v) crow.ii.wslash.event(i,v) end



_norns.crow = {}

_norns.crow.add = function(id, name, dev)
  norns.crow.dev = dev
  crow.add(id, name, dev)
end

_norns.crow.remove = function(id)
  norns.crow.dev = nil
  crow.remove(id)
end

_norns.crow.event = function(id, line)
  line = string.sub(line,1,-2) -- strip newline
  --print(line)
  if util.string_starts(line,"^^") == true then
    line = line:gsub("%^^","norns.crow.")
    assert(load(line))()
  else
    crow.receive(line)
  end
end


-- ----

local input = {}

function input.new(x)
  local i = { n = x }
  i.query = function() crow.send("get_cv("..i.n..")") end
  i.stream = function(v) print("crow input stream: "..i.n.." "..v) end
  i.change = function(v) print("crow input change: "..i.n.." "..v) end
  i.mode = function(m,a,b,c)
    local cmd = "input["..i.n.."].mode("..tostringwithquotes(m)
    if a ~= nil then cmd = cmd .. "," .. a end
    if b ~= nil then cmd = cmd .. "," .. b end
    if c ~= nil then cmd = cmd .. "," .. tostringwithquotes(c) end
    cmd = cmd .. ")"
    crow.send(cmd)
  end
  setmetatable(i,input)
  return i
end

setmetatable(input, input)


local output = {}

function output.new(x)
  local o = { n = x }
  o._volts = 0
  o._slew = 0
  o.query = function() crow.send("get_out("..o.n..")") end
  o.receive = function(v) print("crow output receive: "..o.n.." "..v) end
  -- WILL BE DEPRECATED in 2.3.0
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

output.__call = function(self,arg)
  local args = ""
  if type(arg) == "string" then -- asl directive
    if arg == "start"
    or arg == "restart"
    or arg == "attack"
    or arg == "release"
    or arg == "step"
    or arg == "unlock" then
      args = tostringwithquotes(arg)
    else -- boolean or asl
      args = arg
    end
  end
  crow.send("output["..self.n.."]("..args..")")
end



setmetatable(output, output)

-- ----

local crow = {}

function crow.version() crow.send("^^v") end
function crow.identity() crow.send("^^i") end
function crow.reset() crow.send("crow.reset()") end
function crow.kill() crow.send("^^k") end
function crow.clear() crow.send("^^c") end

function crow.send(cmd)
  if norns.crow.dev then
    --print("crow send: "..cmd)
    _norns.crow_send(norns.crow.dev,cmd)
  end
end

function crow.add(id, name, dev) print(">>>>>> norns.crow.add / " .. id .. " / " .. name) end
function crow.remove(id) print(">>>>>> norns.crow.remove " .. id) end
function crow.receive(...) print("crow:",...) end

crow.input = { input.new(1), input.new(2) }
crow.output = { output.new(1), output.new(2), output.new(3), output.new(4) }

crow.init = function()
  crow.reset()
  crow.add = function(id, name, dev) print(">>>>>> norns.crow.add / " .. id .. " / " .. name) end
  crow.remove = function(id) print(">>>>>> norns.crow.remove " .. id) end
  crow.receive = function(...) print("crow:",...) end
  crow.input = { input.new(1), input.new(2) }
  crow.output = { output.new(1), output.new(2), output.new(3), output.new(4) }
  crow.midi = function(...) print("crow midi:",...) end

  crow.ii.ansible.event = function(i,v) print("ansible ii: "..i.." "..v) end
  crow.ii.kria.event = function(i,v) print("kria ii: "..i.." "..v) end
  crow.ii.meadowphysics.event = function(i,v) print("mp ii: "..i.." "..v) end
  crow.ii.wslash.event = function(i,v) print("wslash ii: "..i.." "..v) end
end

crow.connected = function()
  return _norns.crow.dev ~= nil
end

crow.ii = {}
crow.ii.pullup = function(x)
  if x == true then crow.send("ii.pullup(true)")
  else crow.send("ii.pullup(false)") end
end

crow.ii.jf = {}
crow.ii.jf.trigger = function(ch,state) crow.send("ii.jf.trigger("..ch..","..state..")") end
crow.ii.jf.run_mode = function(mode) crow.send("ii.jf.run_mode("..mode..")") end
crow.ii.jf.run = function(volts) crow.send("ii.jf.run("..volts..")") end
crow.ii.jf.transpose = function(pitch) crow.send("ii.jf.transpose("..pitch..")") end
crow.ii.jf.vtrigger = function(ch,level) crow.send("ii.jf.vtrigger("..ch..","..level..")") end
crow.ii.jf.retune = function(ch,numerator,denominator) crow.send("ii.jf.retune("..ch..","..numerator..","..denominator..")") end
crow.ii.jf.mode = function(mode) crow.send("ii.jf.mode("..mode..")") end
crow.ii.jf.play_voice = function(channel,pitch,level) crow.send("ii.jf.play_voice("..channel..","..pitch..","..level..")") end
crow.ii.jf.play_note = function(pitch,level) crow.send("ii.jf.play_note("..(pitch)..","..(level)..")") end
crow.ii.jf.god_mode = function(state) crow.send("ii.jf.god_mode("..state..")") end
crow.ii.jf.tick = function(clock) crow.send("ii.jf.tick("..clock..")") end
crow.ii.jf.quantize = function(divisions) crow.send("ii.jf.quantize("..divisions..")") end

crow.ii.wslash = {}
crow.ii.wslash.record = function(active) crow.send("ii.wslash.record("..active..")") end
crow.ii.wslash.play = function(direction) crow.send("ii.wslash.play("..direction..")") end
crow.ii.wslash.loop = function(state) crow.send("ii.wslash.loop("..state..")") end
crow.ii.wslash.cue = function(destination) crow.send("ii.wslash.cue("..destination..")") end

crow.ii.ansible = {}
crow.ii.ansible.trigger = function(channel, state) crow.send("ii.ansible.trigger("..channel..","..state..")") end
crow.ii.ansible.trigger_toggle = function(channel) crow.send("ii.ansible.trigger_toggle("..channel..")") end
crow.ii.ansible.trigger_pulse = function(channel) crow.send("ii.ansible.trigger_pulse("..channel..")") end
crow.ii.ansible.trigger_time = function(channel, time) crow.send("ii.ansible.trigger_time("..channel..","..time..")") end
crow.ii.ansible.trigger_polarity = function(channel, polarity) crow.send("ii.ansible.trigger_polarity("..channel..","..polarity..")") end
crow.ii.ansible.cv = function(channel, volts) crow.send("ii.ansible.cv("..channel..","..volts..")") end
crow.ii.ansible.cv_slew = function(channel, time) crow.send("ii.ansible.cv_slew("..channel..","..time..")") end
crow.ii.ansible.cv_offset = function(channel, volts) crow.send("ii.ansible.cv_offset("..channel..","..volts..")") end
crow.ii.ansible.cv_set = function(channel, volts) crow.send("ii.ansible.cv_set("..channel..","..volts..")") end

crow.ii.kria = {}
crow.ii.kria.preset = function(number) crow.send("ii.kria.preset("..number..")") end
crow.ii.kria.pattern = function(number) crow.send("ii.kria.pattern("..number..")") end
crow.ii.kria.scale = function(number) crow.send("ii.kria.scale("..number..")") end
crow.ii.kria.period = function(time) crow.send("ii.kria.period("..time..")") end
crow.ii.kria.position = function(track, param, pos) crow.send("ii.kria.position("..track..","..param..","..pos..")") end
crow.ii.kria.loop_start = function(track, param, pos) crow.send("ii.kria.loop_start("..track..","..param..","..pos..")") end
crow.ii.kria.loop_length = function(track, param, pos) crow.send("ii.kria.loop_length("..track..","..param..","..pos..")") end
crow.ii.kria.reset = function(track, param) crow.send("ii.kria.reset("..track..","..param..")") end
crow.ii.kria.mute = function(track, state) crow.send("ii.kria.mute("..track..","..state..")") end
crow.ii.kria.toggle_mute = function(track) crow.send("ii.kria.toggle_mute("..track..")") end
crow.ii.kria.clock = function(track) crow.send("ii.kria.clock("..track..")") end

crow.ii.meadowphysics = {}
crow.ii.meadowphysics.preset = function(number) crow.send("ii.meadowphysics.preset("..number..")") end
crow.ii.meadowphysics.reset = function(track) crow.send("ii.meadowphysics.reset("..track..")") end
crow.ii.meadowphysics.stop = function(track) crow.send("ii.meadowphysics.stop("..track..")") end
crow.ii.meadowphysics.scale = function(number) crow.send("ii.meadowphysics.scale("..number..")") end
crow.ii.meadowphysics.period = function(time) crow.send("ii.meadowphysics.period("..time..")") end
crow.ii.meadowphysics.get = function(param) crow.send("ii.meadowphysics.get('"..param.."')") end


return crow
