-- STARTUP

require 'math'

require 'engine'
require 'grid'
require 'hid'
require 'midi'
require 'poll'
require 'metro'
require 'menu'

-- more random
math.randomseed(os.time())

-- globals
s = require 'screen'
grid = require 'grid'
metro = require 'metro'
poll = require 'poll'
e = require 'engine'
wifi = require 'wifi'

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
  _, g = next(grid.devices) -- hacky way to get basically random item in a table
  if g then grid.add(g) end
end

grid.remove = function(device) g = nil end

-- resume last loaded script
norns.log.post("norns started")
norns.state.resume()
