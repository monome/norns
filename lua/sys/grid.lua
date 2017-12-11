--- Grid class
-- @module grid
-- @alias Grid
print('grid.lua')
require 'norns'
norns.version.grid = '0.0.2'


---------------------------------
-- Grid device class

local Grid = {}
Grid.devices = {}
Grid.__index = Grid

--- constructor
-- @tparam integer id : arbitrary numeric identifier
-- @tparam string serial : serial
-- @tparam string name : name
-- @tparam userdata dev : opaque pointer to device
function Grid.new(id, serial, name, dev)
   local g = setmetatable({}, Grid)
   g.id = id
   g.serial = serial
   g.name = name
   g.dev = dev -- opaque pointer
   g.key = nil -- key event callback
   g.remove = nil -- device unplug callback
   return g
end

--- static callback when any grid device is added; 
-- user scripts can redefine
-- @param dev : a Grid table
function Grid.add(dev)
   print("grid added", dev.id, dev.name)
end

--- static callback when any grid device is removed; 
-- user scripts can redefine
-- @param dev : a Grid table
function Grid.remove(dev)
   -- print("Grid.add")
   -- dev:print()
end

--- set state of single LED on this grid device
-- @tparam integer x : column index (1-based!)
-- @tparam integer y : row index (1-based!)
-- @tparam integer val : LED brightness in [1, 16]
function Grid:led(x, y, val)
   grid_set_led(self.dev, x, y, val)
end

--- set state of all LEDs on this grid device
-- @tparam integer val : LED brightness in [1, 16]
function Grid:all(val)
   grid_all_led(self.dev, val)
end

--- update any dirty quads on this grid device
function Grid:refresh()
   grid_refresh(self.dev)
end

--- print a description of this grid device
function Grid:print()
   for k,v in pairs(self) do
      print('>> ', k,v)
   end
end

-- -------------------------------
-- monome device manager

-- @fixme shouldn'e be in this module, shouldn't assume all monomes are grids

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
norns.grid.add = function(id, serial, name, dev)
   local g = Grid.new(id,serial,name,dev)
   Grid.devices[id] = g
   if Grid.add ~= nil then Grid.add(g) end
end

norns.grid.remove = function(id)
   if Grid.devices[id] then
      if Grid.remove ~= nil then
	 Grid.remove(Grid.devices[id])
      end
   end
   Grid.devices[id] = nil
end

--- redefine global grid key input handler
norns.grid.key = function(id, x, y, val)
   local g = Grid.devices[id]
   if g ~= nil then
      if g.key ~= nil then
	 g.key(x, y, val)
      end
   else
      print('>> error: no entry for grid ' .. id)
   end
end

return Grid
