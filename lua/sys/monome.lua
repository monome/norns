norns.version.monome = '0.0.1'

---------------------------------
-- Grid device class

Grid = {}
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

----------------------------------
-- Arc device class

Arc = {}
Arc.__index = Arc

-- TODO...

---------------------------------
-- monome device manager

norns.monome = {}

norns.monome.add = function(id, serial, name, dev)
   -- TODO: distinguish between grids and arcs
   -- for now, assume its a grid
   norns.grid.add(id,serial,name,dev)
end

-- grid devices
norns.grid = {}
grid = {} -- <-- script callbacks go in here

norns.grid.add = function(id, serial, name, dev)
   print('>> adding device')
   local m = Grid:new(id,serial,name,dev)
   norns.grid[id] = m
   if grid.add ~= nil then grid.add(m) end
end

norns.grid.remove = function(id)
   norns.grid[id] = nil
   if grid.remove ~= nil then grid.remove(m) end
end

-- grid key input handler
-- first argument is the device id
norns.grid.key = function(id, x, y, val)
   local g = norns.grid[id]
   if g ~= nil then
	  if grid.key ~= nil then grid.key(g, x, y, val) end
   else
	  print('>> error: no entry for grid ' .. id)
   end
end

-- print all grids
norns.grid.print = function()
   for id,gr in norns.grid do
	  gr:print()
   end
end

-- TODO: arc devices
norns.arc = {}
