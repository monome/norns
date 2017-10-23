--[[
   norns.lua
   startup script for norns lua environment.

   NB: removal of any function/module definitions will break the C->lua glue.
   in other respects, feel free to customize.
--]]


--[[
   the 'norns' global table contains all system-level objects,
   in particular the callbacks expected by the C code.

   many of these wrap script-defined callbacks;
   e.g. `norns.input.add()` executes `input.add()` if it's defined,
   after performing system-level bookkeeping tasks. 
--]]

norns = {}
norns.version = {}
norns.version.norns = "0.0.1"

print("running norns.lua")

require('helpers')
require('input')
require('monome')

-- this function will be run after I/O subsystems are initialized,
-- but before I/O event loop starts ticking
startup = function()
   require('startup')
end

-- table of engine commands
norns.engine = {}

-- table of handlers for descriptor reports
norns.report = {}
report = {} -- <-- script-defined callbacks go in here

norns.report.engines = function(names, count)
   print(count .. " engines: ")
   for i=1,count do
	  print(i .. ": "..names[i])
   end
   if report.engines ~= nil then report.engines(names, count) end
end

norns.report.commands = function(commands, count)
   print("norns.report.commands")
   addEngineCommands(commands, count)
   -- call the script-defined report callback, if it exists
   -- this is helpful for a script to continue execution once an engine is loaded.
   if report.commands ~= nil then report.commands(commands, count) end
end

norns.polls = {}
norns.pollNames = {}
norns.report.polls = function(polls, count)
   print("norns.report.polls ; count: " .. count)
   -- call the script-defined report callback, if it exists
   if report.polls ~= nil then report.polls(polls, count) end
   norns.polls = {}
   norns.pollNames = {}
   local t, name, idx
   for i=1,count do
      print("poll " .. polls[i][1] .. " : " .. polls[i][2] .. " type: " .. polls[i][3])
      idx = polls[i][1]
      name = polls[i][2]
      norns.polls[name] = {}
      norns.polls[name].idx = polls[i][1]
      norns.polls[name].name = polls[i][2]
      norns.polls[name].type= polls[i][3]
      norns.pollNames[idx] = name;
   end
   -- call the script-defined report callback, if it exists
   if report.polls ~= nil then report.polls(polls, count) end
end

norns.poll = function(idx, arg)
   --- FIXME: testing
   local name = nil
--   print("norns.poll: "..idx .. " "..arg)
   if norns.pollNames[idx] ~= nil then
      name = norns.pollNames[idx] 
      -- print("norns.poll: "..idx..": " ..name..": "..arg)
      -- call script-defined poll callback
      if poll ~= nil then poll(norns.polls[name], arg) end
   else
      print("norns.poll: unknown index: " .. idx .. " "..arg)
   end
end

norns.timer = function(idx, stage)
   -- call script-defined timer callback
   print("norns.timer: ".. idx.." "..stage)
   if timer ~= nil then timer(idx,stage) end
end

norns.version_print = function()
  for key,value in pairs(norns.version) do
    print(key .. ": "  .. value)
  end
end

norns.load_script = function()
   -- TODO? kill_all_timers();
end

norns.start_poll = function(pollName)
   local p = norns.polls[pollName]
   if p ~= nil then 
      start_poll(p.idx)
   end
end

norns.stop_poll = function(pollName)
   local p = norns.polls[pollName]
   if p ~= nil then 
      stop_poll(p.idx)
   end
end

norns.set_poll_time = function(pollName, t)
   local p = norns.polls[pollName]
   if p ~= nil then 
      set_poll_time(p.idx, t)
   end
end
