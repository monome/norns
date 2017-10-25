require 'norns'

---------------------------------
-- Grid device class

local Grid = {}
Grid.__index = Grid

function Grid:new(id, serial, name, dev)
   local g = setmetatable({}, Grid)
   g.id = id
   g.serial = serial
   g.name = name
   g.dev = dev -- opaque pointer
   return g
end

function Grid:led(x, y, val)
   grid_set_led(self.dev, x, y, val)
end

function Grid:refresh(x, y, val)
   grid_refresh(self.dev)
end

function Grid:print()
   for k,v in pairs(self) do
      print('>> ', k,v)
   end
end

---------------------------------
-- monome device manager

norns.monome = {}

norns.monome.add = function(id, serial, name, dev)
   -- TODO: distinguish between grids and arcs
   -- for now, assume its a grid
   norns.grid.add(id,serial,name,dev)
end


norns.monome.remove = function(id)
   -- TODO: distinguish between grids and arcs
   -- for now, assume its a grid
   norns.grid.remove(id)
end

-- grid devices
norns.grid = {}
norns.grid.devices = {}
grid = {} -- <-- script callbacks go in here

norns.grid.add = function(id, serial, name, dev)
   print('>> adding monome device')
   local m = Grid:new(id,serial,name,dev)
   m:print()
   norns.grid.devices[id] = m
   if grid.add ~= nil then grid.add(m) end
end

norns.grid.remove = function(id)
   print('>> removing monome device ' .. id)
   if grid.remove ~= nil then grid.remove(norns.grid.devices[id]) end
   norns.grid.devices[id] = nil
end

-- grid key input handler
-- first argument is the device id
norns.grid.key = function(id, x, y, val)
   local g = norns.grid.devices[id]
   if g ~= nil then
	  if grid.key ~= nil then grid.key(g, x, y, val) end
   else
	  print('>> error: no entry for grid ' .. id)
   end
end

-- print all grids
norns.grid.print = function()
   for id,gr in norns.grid.devices do
	  gr:print()
   end
end

return 
