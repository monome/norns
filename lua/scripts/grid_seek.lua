print('grid_seek.lua')
require 'norns'
local grid = require 'grid'
local metro = require 'metro'

e = require 'engine'

e.load('TestSine',
  function(commands, count)
    print("sine loaded")
    e.hz(100)
    e.amp(0.25)
  end
)

g = nil -- grid device

-- global vars should be cleaned up when the next script loads
norns.script.cleanup = function()
   g = nil
   t:stop()
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
   assert(g) -- its an error to receive callback if we have no device
   if state > 0 then 
      steps[x] = y
   end
   g:refresh()
end

t = metro[1]
t.time = 0.1

pos = 1

steps = {1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8}
notes = {400,350,300,250,200,150,125,100}

t.callback = function(stage)
  pos = pos + 1
  if pos == 17 then pos = 1 end
  e.hz(notes[steps[pos]])
  if g ~= nil then redraw() end
end

redraw = function()
  --[[for x = 1,16 do
    for y = 1,8 do
      g:led(x,y,0)
    end
  end
  --]]

  g:all(1)

  for x = 1,16 do g:led(x,steps[x],5) end

  g:led(pos,steps[pos],15)

  g:refresh();
end


if e.hz then t:start() end
