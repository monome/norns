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


crow.ii = {}
crow.ii.pullup = function(x) if x == true then crow.send("ii.pullup(true)")
  else crow.send("ii.pullup(false)") end end
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

crow.ii.ansible = {}
crow.ii.ansible.trigger = function(channel, state) crow.send("ii.ansible.trigger("..channel..","..state..")") end
crow.ii.ansible.trigger_toggle = function(channel) crow.send("ii.ansible.trigger_toggle("..channel..")") end
crow.ii.ansible.trigger_pulse = function(channel) crow.send("ii.ansible.trigger_toggle("..channel..")") end
crow.ii.ansible.trigger_time = function(channel, time) crow.send("ii.ansible.trigger_time("..channel..","..time..")") end
crow.ii.ansible.trigger_polarity = function(channel, polarity) crow.send("ii.ansible.trigger_polarity("..channel..","..polarity..")") end
crow.ii.ansible.cv = function(channel, volts) crow.send("ii.ansible.cv("..channel..","..volts..")") end
crow.ii.ansible.cv_slew = function(channel, time) crow.send("ii.ansible.cv_slew("..channel..","..time..")") end
crow.ii.ansible.cv_offset = function(channel, volts) crow.send("ii.ansible.cv_offset("..channel..","..volts..")") end
crow.ii.ansible.cv_set = function(channel, volts) crow.send("ii.ansible.cv_set("..channel..","..volts..")") end

crow.ii.ansible_kria = {}
crow.ii.ansible_kria.preset = function(number) crow.send("ii.ansible_kria.preset("..number..")") end
crow.ii.ansible_kria.pattern = function(number) crow.send("ii.ansible_kria.pattern("..number..")") end
crow.ii.ansible_kria.scale = function(number) crow.send("ii.ansible_kria.scale("..number..")") end
crow.ii.ansible_kria.period = function(time) crow.send("ii.ansible_kria.period("..time..")") end
crow.ii.ansible_kria.position = function(track, param, pos) crow.send("ii.ansible_kria.position("..track..","..param..","..pos..")") end
crow.ii.ansible_kria.loop_start = function(track, param, pos) crow.send("ii.ansible_kria.loop_start("..track..","..param..","..pos..")") end
crow.ii.ansible_kria.loop_length = function(track, param, pos) crow.send("ii.ansible_kria.loop_length("..track..","..param..","..pos..")") end
crow.ii.ansible_kria.reset = function(track, param) crow.send("ii.ansible_kria.reset("..track..","..param..")") end
crow.ii.ansible_kria.mute = function(track, state) crow.send("ii.ansible_kria.mute("..track..","..state..")") end
crow.ii.ansible_kria.toggle_mute = function(track) crow.send("ii.ansible_kria.toggle_mute("..track..")") end
crow.ii.ansible_kria.clock = function(track) crow.send("ii.ansible_kria.clock("..track..")") end

crow.ii.ansible_mp = {}
crow.ii.ansible_mp.preset = function(number) crow.send("ii.ansible_mp.preset("..number..")") end
crow.ii.ansible_mp.reset = function(track) crow.send("ii.ansible_mp.reset("..track..")") end
crow.ii.ansible_mp.stop = function(track) crow.send("ii.ansible_mp.stop("..track..")") end
crow.ii.ansible_mp.scale = function(number) crow.send("ii.ansible_mp.scale("..number..")") end
crow.ii.ansible_mp.period = function(time) crow.send("ii.ansible_mp.period("..time..")") end



return crow
