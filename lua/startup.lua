-- STARTUP 

require 'menu'
require 'monome'

require 'math' 
math.randomseed(os.time()) -- more random

-- globals
screen = require 'screen'
grid = require 'grid'
_arc = require 'arc'
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

-- management of grids
g = nil

grid.add = function(device)
  print("attaching grid ")
  g = device
  g.key = gridkey
  g:print()
  norns.log.post("connected: grid")
end

grid.reconnect = function()
   print("grid.reconnect (default)")
  _, g = next(grid.devices) -- hacky way to get basically random item in a table
  if g then
     grid.add(g)
  end
end

grid.remove = function(device) g = nil end

-- management of arcs
arc = nil

_arc.add = function(device)
  print("attaching arc ")
  arc = device
  arc.enc = arcenc
  arc:print()
  norns.log.post("connected: arc")
end

_arc.reconnect = function()
   print("arc.reconnect (default)")
  _, arc = next(_arc.devices) -- hacky way to get basically random item in a table
  if arc then
     _arc.add(arc)
  end
end

_arc.remove = function(device) arc = nil end

print("setting startup_status callbacks...")

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


-- check for pending updates
norns.update.run()

print("start_audio(): ")
-- start the process of syncing with crone boot
start_audio()
