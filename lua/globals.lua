-- GLOBALS 
s = require 'screen' 
grid = require 'grid'
metro = require 'metro'
poll = require 'poll'
e = require 'engine'
g = nil 

grid.add = function(device)
   print("attaching grid ")
   g = device
   g.key = gridkey
   g:print()
   sys.log.post("connected: grid")
end 

grid.reconnect = function()
    _, g = next(grid.devices) -- hacky way to get basically random item in a table
    if g then grid.add(g) end 
end

grid.remove = function(device) g = nil end


sys = {}

sys.none = function() end
sys.blank = function() s_clear() end

sys.script = require 'script' 
sys.log =  require 'log'
sys.file = require 'file'

key = sys.none
enc = sys.none
redraw = sys.blank
cleanup = sys.none 

sys.input_left = 20 --default, overwritten by saved state
sys.input_right = 20 --default, overwritten by saved state
sys.hp = 20 -- default 

-- SHORTCUTS
sys.run = sys.script.load
sys.time = get_time 

-- HELPERS
require 'math'
math.randomseed(os.time()) 
clamp = function(n, min, max) return math.min(max,(math.max(n,min))) end
tab = require 'tabutil'
