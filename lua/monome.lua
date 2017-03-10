print("running monome.lua")

---------------------------------
-- Grid device device class

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

monome = {}
monome.grids = {}
monome.arcs = {}

monome.add = function(id, serial, name, dev)
   print('>> adding device')
   local m = Grid:new(id,serial,name,dev)
   -- TODO: decide if this is a Grid or an Arc
   -- for now, assume it's a grid
   m:print()
   monome.grids[id] = m;
end

monome.remove = function(id)
   print('>> removing device')
   monome.grids[id]:print()
   monome.grids[id] = nil
end

-- key input handler
-- first argument is the device id
key_test_val = 1;
monome.key = function(id, x, y, val)
   print('>> handling key event for grid ' .. id)
   local m = monome.grids[id]
   if m ~= nil then
	  if val > 0 then
		 m:led(x, y, key_test_val)
		 key_test_val = key_test_val + 1
		 if key_test_val > 15 then key_test_val = 1 end
	  else
		 m:led(x, y, 0x0)
	  end
	  m:refresh()
   end
end

monome.print_grids = function()
   for id,gr in monome.grids do
	  gr:print()
   end
end
