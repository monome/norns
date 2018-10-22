-- STARTUP

require 'math'
math.randomseed(os.time()) -- more random

-- globals
screen = require 'screen'
grid = require 'grid'
hid = require 'hid'
metro = require 'metro'
midi = require 'midi'
osc = require 'osc'
poll = require 'poll'
engine = require 'engine'
wifi = require 'wifi'

fileselect = require 'fileselect'
textentry = require 'textentry'

controlspec = require 'controlspec'
paramset = require 'paramset'

params = paramset.new()

tab = require 'tabutil'
util = require 'util'

-- load menu
require 'menu'


norns.startup_status.ok = function()
  print("norns.startup_status.ok")
  -- resume last loaded script
  norns.script.clear()
  norns.log.post("norns started")
  norns.state.resume()
end

norns.startup_status.timeout = function()
  print("norns.startup_status.timeout")
  norns.script.clear()
  norns.scripterror("AUDIO ENGINE")
end

-- initial screen state
s_save()

-- check for pending updates
norns.update.run()

print("start_audio(): ")
-- start the process of syncing with crone boot
start_audio()
