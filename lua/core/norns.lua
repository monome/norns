-- norns.lua
-- main norns script, called by matron
-- defines top-level global tables and functions needed by other modules
-- external c functions are in the _norns table
-- external callbacks in the norns table, which also includes management

--- System utilities
-- @module norns
norns = {}

local engine = require 'core/engine'
local poll = require 'core/poll'
local tab = require 'tabutil'
local util = require 'util'
local hook = require 'core/hook'

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

_norns.softcut_render = function(ch, start, sec_per_sample, samples) end
_norns.softcut_position = function(i,pos) end

-- default readings for battery
norns.battery_percent = 0
norns.battery_current = 0

-- battery percent handler
_norns.battery = function(percent, current)
  if current < 0 and percent < 5 and norns.state.battery_warning==1 then
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
_norns.stat = function(disk, temp, cpu, cpu1, cpu2, cpu3, cpu4)
  --print("stat",disk,temp,cpu)
  norns.disk = disk
  norns.temp = temp
  norns.cpu_avg = cpu
  norns.cpu = {}
  norns.cpu[1] = cpu1
  norns.cpu[2] = cpu2
  norns.cpu[3] = cpu3
  norns.cpu[4] = cpu4
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

--- do nothing.
norns.none = function() end

--- draw a blank screen.
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

--- shutdown
norns.shutdown = function()
  hook.system_pre_shutdown()
  print("SLEEP")
  --TODO fade out screen then run the shutdown script
  norns.state.clean_shutdown = true
  norns.state.save()
  pcall(cleanup)
  audio.level_dac(0)
  audio.headphone_gain(0)
  os.execute("sleep 0.5; sudo shutdown now")
end

--- platform detection
-- 0 = UNKNOWN
-- 1 = OTHER
-- 2 = CM3 (norns)
-- 3 = PI3 (norns shield)
norns.platform = _norns.platform()

--- true if we are running on norns (CM3)
norns.is_norns = norns.platform == 2

--- true if we are running on norns shield (PI3)
norns.is_shield = norns.platform == 3

-- Util (system_cmd)
local system_cmd_q = {}
local system_cmd_busy = false

--- add cmd to queue
-- @tparam string cmd shell command to execute
-- @tparam ?func callback the callback will be called with the output of the
-- command after it completes. if the callback is nil, then print the output
-- instead.
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

--- find pathnames matching a pattern
-- @function system_glob
-- @tparam string pattern
-- @treturn {string,...} a table of matching pathnames
norns.system_glob = _norns.system_glob

-- audio reset
_norns.reset = function()
  os.execute("sudo systemctl restart norns-sclang.service")
  os.execute("sudo systemctl restart norns-crone.service")
  os.execute("sudo systemctl restart norns-matron.service")
end

-- restart device
_norns.restart = function()
  hook.system_pre_shutdown()
  print("RESTARTING")
  norns.script.clear()
  _norns.free_engine()
  norns.state.clean_shutdown = true
  norns.state.save()
  pcall(cleanup)
  audio.level_dac(0)
  audio.headphone_gain(0)
  _norns.reset()
end

-- startup function will be run after I/O subsystems are initialized,
-- but before I/O event loop starts ticking (see readme-script.md)
_startup = function()
  require('core/startup')
end

_post_startup = function()
   print('_norns._post_startup')
   hook.system_post_startup()
end

--- rerun the current script
norns.rerun = function()
  norns.script.load(norns.state.script)
end

-- expand the filesystem after a fresh installation
norns.expand_filesystem = function()
  os.execute('sudo raspi-config --expand-rootfs')
end
