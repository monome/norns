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

-- this function will be run after I/O subsystems are initialized,
-- but before I/O event loop starts ticking
startup = function()
   require('startup')
end

-----------------------------
--- system state tables

------- reports
-- table of handlers for descriptor reports
norns.report = {}
-- table of script-defined report callbacks
report = {}

----- commands
-- table of engine commands
norns.engine = {}

----- polls
-- poll objects (index by name)
norns.polls = {}
-- poll names (indexed by int) - for reverse lookup
norns.pollNames = {}


---------------------------------------------
---- report callbacks

norns.report.engines = function(names, count)
   print("norns.report.engines")
   --[[
   print(count .. " engines: ")
   for i=1,count do
	  print(i .. ": "..names[i])
   end
   if report.engines ~= nil then report.engines(names, count) end
   --]]
end

norns.report.commands = function(commands, count)
   print("norns.report.commands")
   -- this should be redefined when engine is loaded (see engine.lua)
end

norns.report.polls = function(polls, count)
   print("norns.report.polls ; count: " .. count)
--[[
   norns.polls = {}
   norns.pollNames = {}
   local t, name, idx
   for i=1,count do
      print("poll " .. polls[i][1] .. " : " .. polls[i][2] .. " type: " .. polls[i][3])
      idx = polls[i][1]
      name = polls[i][2]

-- FIXME
      
--      norns.polls[name] = Poll(polls[i][1], polls[i][2], polls[i][3])
--      norns.polls[name] = {}
--      norns.polls[name].idx = polls[i][1]
--      norns.polls[name].name = polls[i][2]
--      norns.polls[name].type= polls[i][3]
      norns.pollNames[idx] = name;
   end
   -- call the script-defined report callback, if it exists
   if report.polls ~= nil then report.polls(polls, count) end
--]]
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
   if norns.cleanup then norns.cleanup()
   end
end

return norns
