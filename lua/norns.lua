--- norns.lua
-- main norns script.
-- defines top-level global tables and functions needed by other modules
-- @module norns
norns = {}
norns.version = {}
norns.version.norns = "0.0.2"

local engine = require 'engine'
local poll = require 'poll'

--- startup function will be run after I/O subsystems are initialized, 
-- but before I/O event loop starts ticking
startup = function()
   require('startup')
end

--- Global Functions
-- @section global_functions

-- global functions required by the C interface; 
-- we "declare" these here with placeholders;
-- indeividual modules will redefine them as needed.

-- monome device callbacks
norns.monome = {}
--- monome device added
norns.monome.add = function(id, serial, name, dev)
   -- print("norns.monome.add "..id, serial, name, dev)
end
--- monome device removed
norns.monome.remove = function(id)
   -- print("norns.monome.remove "..id)
end

--- grid device callbacks
norns.grid = {}
--- grid key event
norns.grid.key = function(id, x, y, val)
   -- print("norns.grid.key ", id,x,y,val)
end

norns.hid = {}
--- HID or other input device added
norns.hid.add = function(id, serial, name, types, codes)
   -- print("norns.input.add ", id, serial, name, types, codes)
end
norns.hid.event = function(id, ev_type, ev_code, value)
   -- print("norns.input.event ", id, ev_type, ev_code, value)
end

--- TODO
-- @todo : arc, midi
norns.arc = {}
norns.midi = {}

--- report callbacks
-- @section report

norns.report = {}
norns.report.engines = function(names, count)
   engine.register(names, count)
   engine.listCommands()
end

-- called when all reports are complete after engine load
norns.report.didEngineLoad = function()
   -- engine module should assign callback
end

norns.report.commands = function(commands, count)
   engine.registerCommands(commands, count)
end

norns.report.polls = function(names, count)
   poll.register(names, count)
   Poll.listNames()
end

-- poll callback; used by C interface
-- @param integer id identfier
-- @param value value (float OR sequence of bytes)
norns.poll = function(id, value)
   local name = poll.pollNames[id]
   local p = poll.polls[name]
   -- print(id, name, p)
   if p then
      p:performCallback(value)
   else
      print ("warning: norns.poll callback couldn't find poll")
   end
end

-- I/O level callback
norns.vu = function(in1, in2, out1, out2)
   --print(in1 .. "\t" .. in2 .. "\t" .. out1 .. "\t" .. out2)
end

