-- STARTUP 
require 'menu'

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
   screen.clear()
   screen.aa(1)
   screen.line_width(1)
   screen.level(10)
   screen.move(10, 10)
   screen.text("error: audio timed out")
   screen.update()
   --- FIXME: should probably do something different here,
   --- but menu breaks if we don't try to load a script :/
   norns.state.resume()
end


print("start_audio(): ")
-- start the process of syncing with crone boot
start_audio()
