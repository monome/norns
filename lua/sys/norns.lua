--- norns.lua;
-- main norns script.
-- defines top-level global tables and functions needed by other modules

norns = {}
norns.version = {}
norns.version.norns = "0.0.2"

print("norns.lua")

--- startup function will be run after I/O subsystems are initialized, 
-- but before I/O event loop starts ticking
startup = function()
   require('startup')
end
--- Global Functions
-- @section global_functions

--- global functions required by the C interface; 
-- we "declare" these here with placeholders;
-- indeividual modules will redefine them as needed.

--- monome device callbacks
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

norns.input = {}
--- HID or other input device added
norns.input.add = function(id, serial, name, types, codes)
   -- print("norns.input.add ", id, serial, name, types, codes)
end
norns.input.event = function(id, ev_type, ev_code, value)
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
   assert(false); -- shouldn't happen
end
norns.report.commands = function(commands, count)
   assert(false) -- shouldn't happen
end
norns.report.polls = function(polls, count)
   -- ok, this could happen if we aren't using the poll module
   -- print("norns.report.polls", commands, count)
end

--- print all module versions
norns.version_print = function()
  for key,value in pairs(norns.version) do
    print(key .. ": "  .. value)
  end
end

--- script managment
-- @section script

norns.script = {}
norns.script.cleanup_dummy = function()
   print("norns.script.cleanup() was invoked on script deinit.")
   print("WARNING: you should redefine this in your script, to free any allocated resources.")
end
norns.script.cleanup = norns.script.cleanup_dummy

--- load a user script
-- @param name (string) - name of the script (without extension)
norns.script.load = function(script)
   norns.script.cleanup() -- cleanup the old script
   norns.script.cleanup = norns.script.cleanup.dummy
   dofile(script_dir..script..".lua")
end

return norns
