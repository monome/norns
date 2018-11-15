--- norns.lua
-- main norns script.
-- defines top-level global tables and functions needed by other modules
-- @module norns
norns = {}
norns.version = {}
norns.version.norns = "1.0.0"

print("norns.lua")

-- import update version number
local fd = io.open(os.getenv("HOME").."/version.txt","r")
if fd then
  io.input(fd)
  norns.version.update = io.read()
  io.close(fd)
else
  norns.version.update = "000000"
end

-- needed requires
local engine = require 'engine'
local poll = require 'poll'
local tab = require 'tabutil'

--- Global Functions
-- @section global_functions

-- global functions required by the C interface;
-- we "declare" these here with placeholders;
-- individual modules will redefine them as needed.

--- battery percent handler
-- @param percent battery full percentage
norns.battery = function(percent, current)
  norns.battery_percent = tonumber(percent)
  norns.battery_current = tonumber(current)
  --print("battery: "..norns.battery_percent.."% "..norns.battery_current.."mA")
end

--- power present handler
-- @param present power plug present (0=no,1=yes)
norns.power = function(present)
  norns.powerpresent = present
  --print("power: "..present)
end

--- stat handler
norns.stat = function(disk, temp, cpu)
  --print("stat",disk,temp,cpu)
  norns.disk = disk
  norns.temp = temp
  norns.cpu = cpu
end


--- key callback (redefined in menu)
norns.key = function(n,z)
   --print ("norns.key "..n.." "..z)
end
--- enc callback (redefined in menu)
norns.enc = function(n,delta)
   --print ("norns.enc "..n.." "..delta)
end

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

-- grid device callbacks
norns.grid = {}
--- grid key event
norns.grid.key = function(id, x, y, val)
   -- print("norns.grid.key ", id,x,y,val)
end

-- arc device callbacks
norns.arc = {}
--- arc enc event
norns.arc.enc = function(id, n, delta, types)
     print("norns.arc.enc ", id,n,delta)
end
--- arc key event
norns.arc.key = function(id, n, val, types)
    print("norns.arc.key ", id,x,val)
end

-- hid callbacks
norns.hid = {}
--- HID or other input device added
norns.hid.add = function(id, serial, name, types, codes)
   -- print("norns.input.add ", id, serial, name, types, codes)
end
norns.hid.event = function(id, ev_type, ev_code, value)
   -- print("norns.input.event ", id, ev_type, ev_code, value)
end

-- midi callbacks (defined in midi.lua)
norns.midi = {}

-- osc callbacks (defined in osc.lua)
norns.osc = {}

--- report callbacks
-- @section report

norns.report = {}
norns.report.engines = function(names, count)
   engine.register(names, count)
end

norns.report.commands = function(commands, count)
   engine.register_commands(commands, count)
   engine.list_commands()
end

norns.report.polls = function(names, count)
   poll.register(names, count)
   poll.list_names()
end


--- called when all reports are complete after engine load
norns.report.did_engine_load = function()
   print("norns.report.did_engine_load (default)")
   -- engine module should assign callback
end

--- startup callbacks
-- @section startup

--- startup handlers
norns.startup_status = {}
norns.startup_status.ok = function() print("startup ok") end
norns.startup_status.timeout = function() print("startup timeout") end

--- poll callback; used by C interface
-- @param id identfier
-- @param value value (float OR sequence of bytes)
norns.poll = function(id, value)
   local name = poll.poll_names[id]
   local p = poll.polls[name]
   if p then
    p:perform(value)
   else
    print ("warning: norns.poll callback couldn't find poll")
   end
end

--- I/O level callback
norns.vu = function(in1, in2, out1, out2)
   --print(in1 .. "\t" .. in2 .. "\t" .. out1 .. "\t" .. out2)
end

--- Audio
norns.audio = require 'audio'


--- Management
-- @section management
---- ... whaaat?? why are all of these made global here?
norns.script = require 'script'
norns.state = require 'state'
norns.log = require 'log'
norns.encoders = require 'encoders'
norns.update = require 'update'

norns.enc = norns.encoders.process

--- Error handling
norns.try = function(f,msg)
  local handler = function (err) return err .. "\n" .. debug.traceback() end
  local status, err = xpcall(f, handler)
  if not status then
    norns.scripterror(msg)
    print(err)
  end
  return status
end

--- Null functions
-- @section null

--- do nothing
norns.none = function() end

--- blank screen
norns.blank = function()
  s_clear()
  s_update()
end


--- startup function will be run after I/O subsystems are initialized,
-- but before I/O event loop starts ticking (see readme-script.md)
startup = function()
  print("norns.lua:startup()")
  require('startup')
end
