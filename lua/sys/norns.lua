--- norns.lua
-- main norns script.
-- defines top-level global tables and functions needed by other modules
-- @module norns

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
   assert(false); -- shouldn't happen
end
norns.report.commands = function(commands, count)
   assert(false) -- shouldn't happen
end
norns.report.polls = function(polls, count)
   -- ok, this could happen if we aren't using the poll module
   -- print("norns.report.polls", commands, count)
end

--- state management
-- @section state

norns.state = {}

norns.state.resume = function()
  dofile(script_dir .. '../state.lua')
  print("last file loaded: " .. norns.state.script)
  norns.script.load()
end

norns.state.save = function()
  local last=io.open(script_dir .. "../state.lua","w+")
  io.output(last)
  io.write("-- state\n")
  io.write("norns.state.script = '" .. norns.state.script .. "'\n")
  io.close(last)   
end

--- script managment
-- @section script

norns.script = {}
norns.script.cleanup_default = function()
   print("cleanup (default)")
end

norns.script.cleanup = norns.script.cleanup_default


--- load a script from the /scripts folder
-- @param filename (string) - file to load, no extension. leave blank to reload current file.
norns.script.load = function(filename)
  if filename == nil then
    filename = norns.state.script end
  local filepath = script_dir .. filename .. '.lua'
  print("trying "..filepath)
  local f=io.open(filepath,"r")
  if f==nil then print "no file there"
  else
    io.close(f)
    norns.script.cleanup() -- cleanup the old script
    norns.script.cleanup = norns.script.cleanup_default
    dofile(filepath)
    norns.state.script = filename
    norns.state.save()
    connect()
  end 
end

--- power management
-- @section power

--- redefine battery percent handler
-- @param percent battery full percentage
norns.battery = function(percent)
  print("battery: "..percent.."%")
end

--- redefine power present handler
-- @param present power plug present (0=no,1=yes)
norns.power = function(present)
  print("power: "..present)
end

return norns
