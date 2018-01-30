require 'engine'
require 'grid'
require 'hid'
require 'poll'
require 'metro'
require 'file'
require 'map'
require 'system'
require 'log'

--print("norns module versions: ")
for mod,v in pairs(norns.version) do
    print (mod,v)
end

-- globals
grid = require 'grid'
metro = require 'metro'
poll = require 'poll'
e = require 'engine'
g = nil

-- user startup script
--require 'first' 

-- helper
require 'math'
math.randomseed(os.time())

norns.run = norns.script.load
norns.time = get_time

-- resume last loaded script
norns.log.post("norns started")
norns.state.resume()

-- grab a grid when one shows up
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
