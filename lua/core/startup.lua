-- STARTUP

tab = require 'tabutil'
util = require 'util'

require 'math'
math.randomseed(os.time()) -- more random

-- globals
screen = require 'core/screen'
monome = require 'core/monome'
grid = require 'core/grid'
arc = require 'core/arc'
hid = require 'core/hid'
metro = require 'core/metro'
midi = require 'core/midi'
osc = require 'core/osc'
poll = require 'core/poll'
engine = tab.readonly{table = require 'core/engine', except = {'name'}}
wifi = require 'core/wifi'

controlspec = require 'core/controlspec'
paramset = require 'core/paramset'
params = paramset.new()


-- load menu
require 'core/menu'


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
