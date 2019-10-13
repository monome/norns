-- STARTUP

tab = require 'tabutil'
util = require 'util'

require 'math'
math.randomseed(os.time()) -- more random

-- globals
audio = require 'core/audio'
screen = require 'core/screen'
monome = require 'core/monome'
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


-- sc init callbacks
norns.startup_status.ok = function()
  print("norns.startup_status.ok")
  -- resume last loaded script
  norns.state.resume()
  -- turn on VU
  _norns.poll_start_vu()
  -- report engines
  report_engines()
  wifi.init()
 
end

norns.startup_status.timeout = function()
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
s_save()

print("start_audio(): ")
-- start the process of syncing with crone boot
start_audio()


