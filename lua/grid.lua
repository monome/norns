--- Grid class
-- @module grid
-- @alias Grid
require 'norns'
norns.version.grid = '0.0.2'


---------------------------------
-- Grid device class

local Grid = {}
Grid.devices = {}
Grid.callbacks = {} -- tables per device, with subscription callbacks
Grid.broadcast = {} -- table with callbacks for "all devices"
Grid.reverse = {} -- reverse lookup, names/id
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
  g.rows = grid_rows(dev)
  g.cols = grid_cols(dev)
  -- update callback table
  if not Grid.callbacks[serial] then
    Grid.callbacks[serial] = {}
  end
  g.callbacks = Grid.callbacks[serial]
  -- update reverse lookup
  Grid.reverse[serial] = id
  return g
end

--- static callback when any grid device is added;
-- user scripts can redefine
-- @param dev : a Grid table
function Grid.add(dev)
  print("grid added:", dev.id, dev.name, dev.serial)
end

--- scan device list and grab one, redefined later
function Grid.reconnect()
  -- FIXME should this emulate behavior in midi.lua?
end

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


--- send grid:led to named device
function Grid.led_named(name, x, y, val)
  if Grid.reverse[name] then
    Grid.devices[Grid.reverse[name]]:led(x, y, val)
  end
end

--- send grid:led to all devices
function Grid.led_all(x, y, val)
  for _,device in pairs(Grid.devices) do
    device:led(x, y, val)
  end
end

--- send grid:all to named device
function Grid.all_named(name, val)
  if Grid.reverse[name] then
    Grid.devices[Grid.reverse[name]]:all(val)
  end
end

--- send grid:all to all devices
function Grid.all_all(val)
  for _,device in pairs(Grid.devices) do
    device:all(val)
  end
end

--- send grid:refresh to named device
function Grid.refresh_named(name)
  if Grid.reverse[name] then
    Grid.devices[Grid.reverse[name]]:refresh()
  end
end

--- send grid:refresh to all devices
function Grid.refresh_all()
  for _,device in pairs(Grid.devices) do
    device:refresh()
  end
end


--- create device, returns object with handler and send
function Grid.connect(name)
  local d = {
    handler = function(data) print("grid input") end,
  }
  if name and type(name) == "number" then
    name = norns.state.ports.grid[name]
  end
  if not name or name == "all" then
    d.led = function(x, y, val) Grid.led_all(x, y, val) end
    d.all = function(val) Grid.all_all(val) end
    d.refresh = function() Grid.refresh_all() end
    table.insert(Grid.broadcast, d)
  else
    if not Grid.callbacks[name] then
      Grid.callbacks[name] = {}
      Grid.reverse[name] = nil -- will be overwritten with device insertion
    end
    d.led = function(x, y, val) Grid.led_named(name, x, y, val) end
    d.all = function(val) Grid.all_named(name, val) end
    d.refresh = function() Grid.refresh_named(name) end
    table.insert(Grid.callbacks[name], d)
  end
  return d
end

--- clear handlers
function Grid.cleanup()
  Grid.broadcast = {}
  for name, t in pairs(Grid.callbacks) do
    Grid.callbacks[name] = {}
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
    if Grid.devices[id].remove then
      Grid.devices[id].remove()
    end
  end
  Grid.devices[id] = nil
end

--- redefine global grid key input handler
norns.grid.key = function(id, x, y, val)
  local g = Grid.devices[id]
  if g ~= nil then
    -- do any individual subscribed callbacks
    for _,device in pairs(Grid.callbacks[Grid.devices[id].serial]) do
      device.handler(x,y,val)
    end
    -- do broadcast callbacks
    for _,device in pairs(Grid.broadcast) do
      device.handler(x,y,val)
    end

    --if g.key ~= nil then
      --g.key(x, y, val)
    --end
  else
    print('>> error: no entry for grid ' .. id)
  end
end

return Grid
