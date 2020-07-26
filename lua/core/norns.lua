-- norns.lua
-- main norns script, called by matron
-- defines top-level global tables and functions needed by other modules
-- external c functions are in the _norns table
-- external callbacks in the norns table, which also includes management

norns = {}

local engine = require 'core/engine'
local poll = require 'core/poll'
local tab = require 'tabutil'
local util = require 'util'

-- Global Functions.

-- global functions required by the C interface;
-- we "declare" these here with placeholders;
-- individual modules will redefine them as needed.

-- key callback
_norns.key = function(n,z) end
-- enc callback
_norns.enc = function(n,delta) end

-- grid device callbacks.
_norns.grid = {}
_norns.grid.key = function(id, x, y, val) end

-- arc device callbacks.
_norns.arc = {}
_norns.arc.delta = function(id, n, delta) end
_norns.arc.key = function(id, n, s) end

-- hid callbacks.
_norns.hid = {}
_norns.hid.add = function(id, name, types, codes, dev) end
_norns.hid.event = function(id, ev_type, ev_code, value) end

-- midi callbacks (defined in midi.lua).
_norns.midi = {}

-- osc callbacks (defined in osc.lua)
_norns.osc = {}

-- clock callbacks (defined in clock.lua)
_norns.clock = {}

-- report callbacks
_norns.report = {}
_norns.report.engines = function(names, count)
   engine.register(names, count)
end
_norns.report.commands = function(commands, count)
   engine.register_commands(commands, count)
   engine.list_commands()
end
_norns.report.polls = function(names, count)
   poll.register(names, count)
   poll.list_names()
end


-- called when all reports are complete after engine load.
_norns.report.did_engine_load = function() end

-- startup handlers.
_norns.startup_status = {}
_norns.startup_status.ok = function() print(">>> startup ok") end
_norns.startup_status.timeout = function() print(">>> startup timeout") end

-- poll callback; used by C interface.
_norns.poll = function(id, value)
   local name = poll.poll_names[id]
   local p = poll.polls[name]
   if p then
    p:perform(value)
   else
    print ("warning: norns.poll callback couldn't find poll")
   end
end

-- i/o level callback.
_norns.vu = function(in1, in2, out1, out2) end
-- softcut phase
_norns.softcut_phase = function(id, value) end

-- default readings for battery
norns.battery_percent = 0
norns.battery_current = 0

-- battery percent handler
_norns.battery = function(percent, current)
  if current < 0 and percent < 5 then
    screen.update = screen.update_low_battery
  elseif current > 0 and norns.battery_current < 0 then
    screen.update = screen.update_default
  end
  norns.battery_percent = tonumber(percent)
  norns.battery_current = tonumber(current)
  --print("battery: "..norns.battery_percent.."% "..norns.battery_current.."mA")
end

-- power present handler
_norns.power = function(present)
  norns.powerpresent = present
  --print("power: "..present)
end

-- stat handler
_norns.stat = function(disk, temp, cpu)
  --print("stat",disk,temp,cpu)
  norns.disk = disk
  norns.temp = temp
  norns.cpu = cpu
end


-- management
norns.script = require 'core/script'
norns.state = require 'core/state'
norns.encoders = require 'core/encoders'

_norns.enc = norns.encoders.process

-- extend paths config table
local p = _path
p.this = tab.readonly{
  table = norns.state,
  expose = {'data', 'path', 'lib'}
}
paths = tab.readonly{ table = p }

-- Error handling.
norns.scripterror = function(msg) print(msg) end
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
  _norns.screen_clear()
  _norns.screen_update()
end


-- Version
norns.version = {}
-- import update version number
local fd = io.open(os.getenv("HOME").."/version.txt","r")
if fd then
  io.input(fd)
  norns.version.update = io.read()
  io.close(fd)
else
  norns.version.update = "000000"
end


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
_norns.system_cmd_capture = function(cap)
  if system_cmd_q[1].callback == nil then print(cap)
  else system_cmd_q[1].callback(cap) end
  table.remove(system_cmd_q,1)
  if #system_cmd_q > 0 then
    _norns.system_cmd(system_cmd_q[1].cmd)
  else
    system_cmd_busy = false
  end
end

-- audio reset
_norns.reset = function()
  os.execute("sudo systemctl restart norns-sclang.service")
  os.execute("sudo systemctl restart norns-crone.service")
  os.execute("sudo systemctl restart norns-matron.service")
end

-- startup function will be run after I/O subsystems are initialized,
-- but before I/O event loop starts ticking (see readme-script.md)
_startup = function()
  require('core/startup')
end
