-- norns.lua
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
local engine = require 'core/engine'
local poll = require 'core/poll'
local tab = require 'tabutil'

-- Global Functions.
-- @section global_functions

-- global functions required by the C interface;
-- we "declare" these here with placeholders;
-- individual modules will redefine them as needed.

norns.battery_percent = 0
norns.battery_current = 0

-- battery percent handler.
-- @param percent battery full percentage
-- @param current
norns.battery = function(percent, current)
  if current < 0 and percent < 5 then
    screen.update = screen.update_low_battery
  elseif current > 0 and norns.battery_current < 0 then
    screen.update = screen.update_default
  end
  norns.battery_percent = tonumber(percent)
  norns.battery_current = tonumber(current)
  --print("battery: "..norns.battery_percent.."% "..norns.battery_current.."mA")
end

-- power present handler.
-- @param present power plug present (0=no,1=yes)
norns.power = function(present)
  norns.powerpresent = present
  --print("power: "..present)
end

-- stat handler.
norns.stat = function(disk, temp, cpu)
  --print("stat",disk,temp,cpu)
  norns.disk = disk
  norns.temp = temp
  norns.cpu = cpu
end


-- key callback (redefined in menu).
norns.key = function(n,z)
   --print ("norns.key "..n.." "..z)
end
-- enc callback (redefined in menu).
norns.enc = function(n,delta)
   --print ("norns.enc "..n.." "..delta)
end

-- monome device callbacks.
norns.monome = {}
-- monome device added.
norns.monome.add = function(id, serial, name, dev)
   -- print("norns.monome.add "..id, serial, name, dev)
end
-- monome device removed.
norns.monome.remove = function(id)
   -- print("norns.monome.remove "..id)
end

-- grid device callbacks.
norns.grid = {}
-- grid key event.
norns.grid.key = function(id, x, y, val)
   -- print("norns.grid.key ", id,x,y,val)
end

-- arc device callbacks.
norns.arc = {}
-- arc key event.
norns.arc.event = function(id, n, delta)
   print("norns.arc.delta ", id, n, delta)
end
norns.arc.key = function(id, n, s)
   print("norns.arc.delta ", id, n, s)
end


-- hid callbacks.
norns.hid = {}
-- HID or other input device added.
norns.hid.add = function(id, name, types, codes, dev)
   -- print("norns.input.add ", id, name, types, codes, dev)
end
norns.hid.event = function(id, ev_type, ev_code, value)
   -- print("norns.input.event ", id, ev_type, ev_code, value)
end

-- midi callbacks (defined in midi.lua).
norns.midi = {}

-- osc callbacks (defined in osc.lua)
norns.osc = {}

-- report callbacks
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


-- called when all reports are complete after engine load.
norns.report.did_engine_load = function()
   print("norns.report.did_engine_load (default)")
   -- engine module should assign callback
end

-- startup callbacks.
-- @section startup

-- startup handlers.
norns.startup_status = {}
norns.startup_status.ok = function() print("startup ok") end
norns.startup_status.timeout = function() print("startup timeout") end

-- poll callback; used by C interface.
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

-- I/O level callback.
norns.vu = function(in1, in2, out1, out2)
   --print(in1 .. "\t" .. in2 .. "\t" .. out1 .. "\t" .. out2)
end

norns.softcut_phase = function(id, value)
  -- print(id,value)
end
-- Audio.
norns.audio = require 'core/audio'

-- Util (system_cmd)
local system_cmd_q = {}
local system_cmd_busy = false

-- add cmd to queue
norns.system_cmd = function(cmd, callback)
  table.insert(system_cmd_q, {cmd=cmd, callback=callback})
  if system_cmd_busy == false then
    system_cmd_busy = true
    _norns.system_cmd(cmd)
  end
end

-- callback management from c
norns.system_cmd_capture = function(cap)
  if system_cmd_q[1].callback == nil then print(cap)
  else system_cmd_q[1].callback(cap) end
  table.remove(system_cmd_q,1)
  if #system_cmd_q > 0 then
    _norns.system_cmd(system_cmd_q[1].cmd)
  else
    system_cmd_busy = false
  end
end


-- Management.
-- @section management

-- why are all of these made global here?
norns.script = require 'core/script'
norns.state = require 'core/state'
norns.encoders = require 'core/encoders'

norns.enc = norns.encoders.process

-- Error handling.
norns.try = function(f,msg)
  local handler = function (err) return err .. "\n" .. debug.traceback() end
  local status, err = xpcall(f, handler)
  if not status then
    norns.scripterror(msg)
    print(err)
  end
  return status
end

-- Null functions.
-- @section null

-- do nothing.
norns.none = function() end

-- blank screen.
norns.blank = function()
  s_clear()
  s_update()
end


-- startup function will be run after I/O subsystems are initialized,
-- but before I/O event loop starts ticking (see readme-script.md)
_startup = function()
  print("norns.lua:startup()")
  require('core/startup')
end
