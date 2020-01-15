--- Crow Module
-- @module crow

local util = require 'util'

local function tostringwithquotes(s)
  return "'"..tostring(s).."'"
end

norns.crow = {}
function norns.crow.identity(...) print("crow identity: " .. ...) end
function norns.crow.version(...) print("crow version: " .. ...) end
function norns.crow.stream(n,v) crow.input[n].stream(v) end
function norns.crow.change(n,v) crow.input[n].change(v) end
function norns.crow.output(n,v) crow.output[n].receive(v) end
function norns.crow.done(n) crow.output[n].done() end
function norns.crow.running(n,v) crow.output[n].running(v) end
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
    crow.send(me..".shape="..tostringwithquotes(v))
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
    --print("crow send: "..cmd)
    _norns.crow_send(norns.crow.dev,cmd)
  end
end
function crow.add(id, name, dev) print(">>>>>> norns.crow.add / " .. id .. " / " .. name) end
function crow.remove(id) print(">>>>>> norns.crow.remove " .. id) end
function crow.receive(...) print("crow:",...) end

crow.input = { input.new(1), input.new(2) }
crow.output = { output.new(1), output.new(2), output.new(3), output.new(4) }

--- initialize
crow.init = function()
  crow.reset()
  crow.add = function(id, name, dev) print(">>>>>> norns.crow.add / " .. id .. " / " .. name) end
  crow.remove = function(id) print(">>>>>> norns.crow.remove " .. id) end
  crow.receive = function(...) print("crow:",...) end
  crow.input = { input.new(1), input.new(2) }
  crow.output = { output.new(1), output.new(2), output.new(3), output.new(4) }
  crow.midi = function(...) print("crow midi:",...) end

  crow.ii.init()
end

--- check if crow is connected
crow.connected = function()
  return _norns.crow.dev ~= nil
end

crow.Cal = {}
crow.Cal.test = function() crow.send("Cal.test()") end
crow.Cal.default = function() crow.send("Cal.default()") end
crow.Cal.print = function() crow.send("Cal.print()") end

crow.ii = {}
crow.ii.help = function() crow.send("ii.help()") end
crow.ii.pullup = function(x)
  local truth = (x == true or x == 1) and 'true' or 'false'
  crow.send("ii.pullup("..truth..")")
end

-- below here is autogenerated with crow utility: github.com/monome/crow/util/ii_norns_support.lua
--  run from crow project root directory: lua util/ii_norns_support.lua lua/ii/ util/norns.lua
--  and copy util/norns.lua below:

crow.ii.init = function()
  crow.ii.ansible.event = function(i,v) print("ansible ii: "..i.." "..v) end
  crow.ii.crow.event = function(i,v) print("crow ii: "..i.." "..v) end
  crow.ii.kria.event = function(i,v) print("kria ii: "..i.." "..v) end
  crow.ii.levels.event = function(i,v) print("levels ii: "..i.." "..v) end
  crow.ii.meadowphysics.event = function(i,v) print("meadowphysics ii: "..i.." "..v) end
  crow.ii.txi.event = function(i,v) print("txi ii: "..i.." "..v) end
  crow.ii.wslash.event = function(i,v) print("wslash ii: "..i.." "..v) end
end

crow.ii.ansible = {}
crow.ii.ansible.help = function() crow.send("ii.ansible.help()") end
crow.ii.ansible.trigger = function(channel,state) crow.send("ii.ansible.trigger("..channel..","..state..")") end
crow.ii.ansible.trigger_toggle = function(channel) crow.send("ii.ansible.trigger_toggle("..channel..")") end
crow.ii.ansible.trigger_pulse = function(channel) crow.send("ii.ansible.trigger_pulse("..channel..")") end
crow.ii.ansible.trigger_time = function(channel,time) crow.send("ii.ansible.trigger_time("..channel..","..time..")") end
crow.ii.ansible.trigger_polarity = function(channel,polarity) crow.send("ii.ansible.trigger_polarity("..channel..","..polarity..")") end
crow.ii.ansible.cv = function(channel,volts) crow.send("ii.ansible.cv("..channel..","..volts..")") end
crow.ii.ansible.cv_slew = function(channel,time) crow.send("ii.ansible.cv_slew("..channel..","..time..")") end
crow.ii.ansible.cv_offset = function(channel,volts) crow.send("ii.ansible.cv_offset("..channel..","..volts..")") end
crow.ii.ansible.cv_set = function(channel,volts) crow.send("ii.ansible.cv_set("..channel..","..volts..")") end

crow.ii.crow = {}
crow.ii.crow.help = function() crow.send("ii.crow.help()") end
crow.ii.crow.output = function(channel,level) crow.send("ii.crow.output("..channel..","..level..")") end
crow.ii.crow.slew = function(channel,time) crow.send("ii.crow.slew("..channel..","..time..")") end
crow.ii.crow.call1 = function(arg) crow.send("ii.crow.call1("..arg..")") end
crow.ii.crow.call2 = function(arg1,arg2) crow.send("ii.crow.call2("..arg1..","..arg2..")") end
crow.ii.crow.call3 = function(arg1,arg2,arg3) crow.send("ii.crow.call3("..arg1..","..arg2..","..arg3..")") end
crow.ii.crow.call4 = function(arg1,arg2,arg3,arg4) crow.send("ii.crow.call4("..arg1..","..arg2..","..arg3..","..arg4..")") end

crow.ii.er301 = {}
crow.ii.er301.help = function() crow.send("ii.er301.help()") end
crow.ii.er301.tr = function(port,state) crow.send("ii.er301.tr("..port..","..state..")") end
crow.ii.er301.tr_tog = function(port) crow.send("ii.er301.tr_tog("..port..")") end
crow.ii.er301.tr_pulse = function(port) crow.send("ii.er301.tr_pulse("..port..")") end
crow.ii.er301.tr_time = function(port,ms) crow.send("ii.er301.tr_time("..port..","..ms..")") end
crow.ii.er301.tr_pol = function(port,rising) crow.send("ii.er301.tr_pol("..port..","..rising..")") end
crow.ii.er301.cv = function(port,volts) crow.send("ii.er301.cv("..port..","..volts..")") end
crow.ii.er301.cv_slew = function(port,ms) crow.send("ii.er301.cv_slew("..port..","..ms..")") end
crow.ii.er301.cv_set = function(port,volts) crow.send("ii.er301.cv_set("..port..","..volts..")") end
crow.ii.er301.cv_off = function(port,volts) crow.send("ii.er301.cv_off("..port..","..volts..")") end

crow.ii.jf = {}
crow.ii.jf.help = function() crow.send("ii.jf.help()") end
crow.ii.jf.trigger = function(channel,state) crow.send("ii.jf.trigger("..channel..","..state..")") end
crow.ii.jf.run_mode = function(mode) crow.send("ii.jf.run_mode("..mode..")") end
crow.ii.jf.run = function(volts) crow.send("ii.jf.run("..volts..")") end
crow.ii.jf.transpose = function(pitch) crow.send("ii.jf.transpose("..pitch..")") end
crow.ii.jf.vtrigger = function(channel,level) crow.send("ii.jf.vtrigger("..channel..","..level..")") end
crow.ii.jf.mode = function(mode) crow.send("ii.jf.mode("..mode..")") end
crow.ii.jf.tick = function(clock_or_bpm) crow.send("ii.jf.tick("..clock_or_bpm..")") end
crow.ii.jf.play_voice = function(channel,pitch_divs,level_repeats) crow.send("ii.jf.play_voice("..channel..","..pitch_divs..","..level_repeats..")") end
crow.ii.jf.play_note = function(pitch_divs,level_repeats) crow.send("ii.jf.play_note("..pitch_divs..","..level_repeats..")") end
crow.ii.jf.god_mode = function(state) crow.send("ii.jf.god_mode("..state..")") end
crow.ii.jf.retune = function(channel,numerator,denominator) crow.send("ii.jf.retune("..channel..","..numerator..","..denominator..")") end
crow.ii.jf.quantize = function(divisions) crow.send("ii.jf.quantize("..divisions..")") end

crow.ii.kria = {}
crow.ii.kria.help = function() crow.send("ii.kria.help()") end
crow.ii.kria.preset = function(number) crow.send("ii.kria.preset("..number..")") end
crow.ii.kria.pattern = function(number) crow.send("ii.kria.pattern("..number..")") end
crow.ii.kria.scale = function(number) crow.send("ii.kria.scale("..number..")") end
crow.ii.kria.period = function(time) crow.send("ii.kria.period("..time..")") end
crow.ii.kria.position = function(track,param,pos) crow.send("ii.kria.position("..track..","..param..","..pos..")") end
crow.ii.kria.loop_start = function(track,param,pos) crow.send("ii.kria.loop_start("..track..","..param..","..pos..")") end
crow.ii.kria.loop_length = function(track,param,pos) crow.send("ii.kria.loop_length("..track..","..param..","..pos..")") end
crow.ii.kria.reset = function(track,param) crow.send("ii.kria.reset("..track..","..param..")") end
crow.ii.kria.mute = function(track,state) crow.send("ii.kria.mute("..track..","..state..")") end
crow.ii.kria.toggle_mute = function(track) crow.send("ii.kria.toggle_mute("..track..")") end
crow.ii.kria.clock = function(track) crow.send("ii.kria.clock("..track..")") end
crow.ii.kria.page = function(page) crow.send("ii.kria.page("..page..")") end
crow.ii.kria.cue = function(pattern) crow.send("ii.kria.cue("..pattern..")") end
crow.ii.kria.direction = function(track,direction) crow.send("ii.kria.direction("..track..","..direction..")") end

crow.ii.levels = {}
crow.ii.levels.help = function() crow.send("ii.levels.help()") end
crow.ii.levels.preset = function(number) crow.send("ii.levels.preset("..number..")") end
crow.ii.levels.reset = function(now) crow.send("ii.levels.reset("..now..")") end
crow.ii.levels.position = function(pos) crow.send("ii.levels.position("..pos..")") end
crow.ii.levels.loop_start = function(pos) crow.send("ii.levels.loop_start("..pos..")") end
crow.ii.levels.loop_length = function(pos) crow.send("ii.levels.loop_length("..pos..")") end
crow.ii.levels.loop_direction = function(direction) crow.send("ii.levels.loop_direction("..direction..")") end

crow.ii.meadowphysics = {}
crow.ii.meadowphysics.help = function() crow.send("ii.meadowphysics.help()") end
crow.ii.meadowphysics.preset = function(number) crow.send("ii.meadowphysics.preset("..number..")") end
crow.ii.meadowphysics.reset = function(track) crow.send("ii.meadowphysics.reset("..track..")") end
crow.ii.meadowphysics.stop = function(track) crow.send("ii.meadowphysics.stop("..track..")") end
crow.ii.meadowphysics.scale = function(number) crow.send("ii.meadowphysics.scale("..number..")") end
crow.ii.meadowphysics.period = function(time) crow.send("ii.meadowphysics.period("..time..")") end

crow.ii.txi = {}
crow.ii.txi.help = function() crow.send("ii.txi.help()") end

crow.ii.wslash = {}
crow.ii.wslash.help = function() crow.send("ii.wslash.help()") end
crow.ii.wslash.record = function(active) crow.send("ii.wslash.record("..active..")") end
crow.ii.wslash.play = function(direction) crow.send("ii.wslash.play("..direction..")") end
crow.ii.wslash.loop = function(state) crow.send("ii.wslash.loop("..state..")") end
crow.ii.wslash.cue = function(destination) crow.send("ii.wslash.cue("..destination..")") end


-- END AUTOGENERATED ALIASES

return crow
