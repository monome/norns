-- STARTUP

require 'math'
math.randomseed(os.time()) -- more random

-- globals
screen = require 'system/lib/screen'
monome = require 'system/lib/monome'
grid = require 'system/lib/grid'
arc = require 'system/lib/arc'
hid = require 'system/lib/hid'
metro = require 'system/lib/metro'
midi = require 'system/lib/midi'
osc = require 'system/lib/osc'
poll = require 'system/lib/poll'
engine = require 'system/lib/engine'
wifi = require 'system/lib/wifi'

controlspec = require 'system/lib/controlspec'
paramset = require 'system/lib/paramset'

params = paramset.new()

tab = require 'tabutil'
util = require 'util'

-- load menu
require 'system/menu'


norns.startup_status.ok = function()
  print("norns.startup_status.ok")
  -- resume last loaded script
  norns.script.clear()
  norns.state.resume()
end

norns.startup_status.timeout = function()
  print("norns.startup_status.timeout")
  norns.script.clear()
  norns.scripterror("AUDIO ENGINE")
end

-- initial screen state
s_save()

print("start_audio(): ")
-- start the process of syncing with crone boot
start_audio()
