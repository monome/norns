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
report = {} -- <-- script callbacks go in here

norns.report.engines = function(names, count)
   print(count .. " engines: ")
   for i=1,count do
	  print(i .. ": "..names[i])
   end
   if report.engines ~= nil then report.engines(names, count) end
end

norns.report.commands = function(commands, count)
   addEngineCommands(commands, count)
   --  call the script-defined report callback.
   -- this is helpful for a script to continue execution once an engine is loaded.
   if report.commands ~= nil then report.commands(commands, count) end
end

norns.timer = function(idx, stage)
   if timer ~= nil then timer(idx,stage) end
end

norns.version_print = function()
  for key,value in pairs(norns.version) do
    print(key .. ": "  .. value)
  end
end
