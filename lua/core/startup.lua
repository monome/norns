-- STARTUP

tab = require 'tabutil'
util = require 'util'

require 'math'
math.randomseed(os.time()) -- more random

-- globals
audio = require 'core/audio'
screen = require 'core/screen'
crow = require 'core/crow'
grid = require 'core/grid'
arc = require 'core/arc'
hid = require 'core/hid'
metro = require 'core/metro'
clock = require "core/clock"
midi = require 'core/midi'
osc = require 'core/osc'
poll = require 'core/poll'
engine = tab.readonly{table = require 'core/engine', except = {'name'}}
softcut = require 'core/softcut'
wifi = require 'core/wifi'
controlspec = require 'core/controlspec'
paramset = require 'core/paramset'
params = paramset.new()
mix = require 'core/mix'
norns.pmap = require 'core/pmap'


-- load menu
require 'core/menu'

-- global include function
function include(file)
  local here = norns.state.path .. file .. '.lua'
  local there = _path.code .. file .. '.lua'
  if util.file_exists(here) then 
    print("including "..here)
    return dofile(here)
  elseif util.file_exists(there) then
    print("including "..there)
    return dofile(there)
  else
    print("### MISSING INCLUDE: "..file)
    error("MISSING INCLUDE: "..file,2)
  end
end

-- monome device management
_norns.monome = {}
_norns.monome.add = function(id, serial, name, dev)
  if util.string_starts(name, "monome arc") then
    _norns.arc.add(id, serial, name, dev)
  else _norns.grid.add(id, serial, name, dev) end
end
_norns.monome.remove = function(id)
  if arc.devices[id] then _norns.arc.remove(id)
  else _norns.grid.remove(id) end
end

-- sc init callbacks
_norns.startup_status.ok = function()
  print("norns.startup_status.ok")
  -- resume last loaded script
  norns.state.resume()
  -- turn on VU
  _norns.poll_start_vu()
  -- report engines
  _norns.report_engines()
  wifi.init()
end

_norns.startup_status.timeout = function()
  norns.script.clear()
  print("norns.startup_status.timeout")
  local cmd="find ~/dust -name *.sc -type f -printf '%p %f\n' | sort -k2 | uniq -f1 --all-repeated=separate"
  local results = util.os_capture(cmd,true)
  if results ~= "" then
    print("DUPLICATE ENGINES:\n" .. results)
    norns.scripterror("DUPLICATE ENGINES")
  else
    norns.scripterror("SUPERCOLLIDER FAIL")
  end
end

-- initial screen state
_norns.screen_save()

-- reverse stereo for norns shield
if util.file_exists(_path.home .. "/reverse.txt") then
  print("NORNS SHIELD: REVERSING STEREO")
  os.execute("jack_disconnect 'crone:output_1' 'system:playback_1'")
  os.execute("jack_disconnect 'crone:output_2' 'system:playback_2'")
  os.execute("jack_connect 'crone:output_1' 'system:playback_2'")
  os.execute("jack_connect 'crone:output_2' 'system:playback_1'")
  os.execute("jack_disconnect 'system:capture_1' 'crone:input_1'")
  os.execute("jack_disconnect 'system:capture_2' 'crone:input_2'")
  os.execute("jack_connect 'system:capture_1' 'crone:input_2'")
  os.execute("jack_connect 'system:capture_2' 'crone:input_1'")
end

print("start_audio(): ")
-- start the process of syncing with crone boot
_norns.start_audio()


