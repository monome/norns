--- Grid class
-- @module grid
-- @alias Grid
require 'norns'


---------------------------------
-- Grid device class

local Grid = {}
Grid.devices = {}
Grid.list = {}
Grid.vport = {}
for i=1,4 do
  Grid.vport[i] = {
    name = "none",
    callbacks = {},
    index = 0,
    led = function() end,
    all = function() end,
    refresh = function() end,
    attached = false
  }
end
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
  name = name .. " " .. serial
  --while tab.contains(Grid.list,name) do
    --name = name .. "+"
  --end
  g.name = name
  g.dev = dev -- opaque pointer
  g.key = nil -- key event callback
  g.remove = nil -- device unplug callback
  g.rows = grid_rows(dev)
  g.cols = grid_cols(dev)
  g.ports = {} -- list of virtual ports this device is attached to

  -- autofill next postiion
  local connected = {}
  for i=1,4 do table.insert(connected, Grid.vport[i].name) end
  if not tab.contains(connected, name) then
    for i=1,4 do
      if Grid.vport[i].name == "none" then
        Grid.vport[i].name = name
        break
      end
    end
  end

  return g
end

--- static callback when any grid device is added;
-- user scripts can redefine
-- @param dev : a Grid table
function Grid.add(dev)
  print("grid added:", dev.id, dev.name, dev.serial)
end

--- scan device list and grab one, redefined later
function Grid.reconnect() end

--- static callback when any grid device is removed;
-- user scripts can redefine
-- @param dev : a Grid table
function Grid.remove(dev) end

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


--- create device, returns object with handler and send
function Grid.connect(n)
  local n = n or 1
  if n>4 then n=4 end

  Grid.vport[n].index = Grid.vport[n].index + 1

  local d = {
    index = Grid.vport[n].index,
    port = n,
    event = function(x,y,z)
        print("grid input")
      end,
    attached = function() return Grid.vport[n].attached end,
    led = function(x,y,z) Grid.vport[n].led(x,y,z) end,
    all = function(val) Grid.vport[n].all(val) end,
    refresh = function() Grid.vport[n].refresh() end,
    disconnect = function(self)
        self.send = function() print("not connected") end
        table.remove(Grid.vport[self.port].callbacks, self.index)
        self.index = nil
        self.port = nil
      end,
    reconnect = function(self, p)
        if self.index then
          table.remove(Grid.vport[self.port].callbacks, self.index)
        end
        self.send = function(data) Grid.vport[p].send(data) end
        attached = function() return Grid.vport[p].attached end
        Grid.vport[p].index = Grid.vport[p].index + 1
        self.index = Grid.vport[p].index
        self.port = p
        Grid.vport[p].callbacks[self.index] = function(data) self.event(data) end
      end
  }

	Grid.vport[n].callbacks[d.index] = function(x,y,z) d.event(x,y,z) end

  return d
end

--- clear handlers
function Grid.cleanup()
  for i=1,4 do
    Grid.vport[i].callbacks = {}
		Grid.vport[i].index = 0
  end
end

function Grid.update_devices()
  -- build list of available devices
  Grid.list = {}
  for _,device in pairs(Grid.devices) do
    table.insert(Grid.list, device.name)
    device.ports = {}
  end
  -- connect available devices to vports
  for i=1,4 do
    Grid.vport[i].attached = false
    Grid.vport[i].led = function(x, y, val) end
    Grid.vport[i].all = function(val) end
    Grid.vport[i].refresh = function() end
    for _,device in pairs(Grid.devices) do
      if device.name == Grid.vport[i].name then
        Grid.vport[i].led = function(x, y, val) device:led(x, y, val) end
        Grid.vport[i].all = function(val) device:all(val) end
        Grid.vport[i].refresh = function() device:refresh() end
        Grid.vport[i].attached = true
        table.insert(device.ports, i)
      end
    end
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
  Grid.update_devices()
  if Grid.add ~= nil then Grid.add(g) end
end

norns.grid.remove = function(id)
  if Grid.devices[id] then
    if Grid.remove ~= nil then
      Grid.remove(Grid.devices[id])
    end
    if Grid.devices[id].remove then
      Grid.devices[id].remove()
    end
  end
  Grid.devices[id] = nil
  Grid.update_devices()
end

--- redefine global grid key input handler
norns.grid.key = function(id, x, y, z)
  local g = Grid.devices[id]
  if g ~= nil then
    if g.key ~= nil then
      g.key(x, y, z)
    end

    for _,n in pairs(g.ports) do
      for _,event in pairs(Grid.vport[n].callbacks) do
        --print("vport " .. n)
        event(x,y,z)
      end
    end
  else
    print('>> error: no entry for grid ' .. id)
  end
end

return Grid
