print('test_grid.lua')
require 'norns'
local grid = require 'grid'

-- variables. not making them local here, so that we can access them in the REPL
g = nil -- grid device
ledVal = 3 -- level of LED output

-- global vars should be cleaned up when the next script loads
norns.cleanup = function()
   g = nil
   ledVal = nil
end

-- function to grab grid device when we find one
setGrid = function (device)
   g = device
   g:print()
   g.key = keyCallback -- set the callback function
end

-- grab a grid if there is one
_, g = next(grid.devices) -- hacky way to get basically random item in a table
print("connected grid: ", g) -- should be nil if grid.devices is empty (e.g. on startup)
if g then setGrid(g) end

-- grab a grid when one shows up
grid.add = function(device)
   print("grabbing new grid ")
   setGrid(device)
end

-- function we assign to grabbed grids
keyCallback = function(x, y, state)
   print("key ", x, y, state)
   assert(g) -- its an error to receive callback if we have no device
   if state > 0 then 
      g:led(x, y, ledVal)
   else
      g:led(x, y, 0)
   end
   g:refresh()
end
