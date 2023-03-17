--- Grid class
-- @module grid

local vport = require 'vport'

local Grid = {}
Grid.__index = Grid

Grid.devices = {}
Grid.vports = {}

for i=1,4 do
  Grid.vports[i] = {
    name = "none",
    device = nil,

    key = nil,
    tilt = nil,
    
    led = vport.wrap_method('led'),
    all = vport.wrap_method('all'),
    refresh = vport.wrap_method('refresh'),
    rotation = vport.wrap_method('rotation'),
    intensity = vport.wrap_method('intensity'),
    tilt_enable = vport.wrap_method('tilt_enable'),
    
    cols = 0,
    rows = 0,
  }
end

-- constructor
-- @tparam integer id : arbitrary numeric identifier
-- @tparam string serial : serial
-- @tparam string name : name
-- @tparam userdata dev : opaque pointer to device
function Grid.new(id, serial, name, dev)
  local g = setmetatable({}, Grid)

  g.id = id
  g.serial = serial
  g.name = name.." "..serial
  g.dev = dev -- opaque pointer
  g.key = nil -- key event callback
  g.tilt = nil -- tilt event callback
  g.remove = nil -- device unplug callback
  g.rows = _norns.grid_rows(dev)
  g.cols = _norns.grid_cols(dev)
  g.port = nil

  -- autofill next postiion
  local connected = {}
  for i=1,4 do
    table.insert(connected, Grid.vports[i].name)
  end
  if not tab.contains(connected, g.name) then
    for i=1,4 do
      if Grid.vports[i].name == "none" then
        Grid.vports[i].name = g.name
        break
      end
    end
  end

  return g
end

--- static callback when any grid device is added;
-- user scripts can redefine
-- @static
-- @param dev : a Grid table
function Grid.add(dev)
  print("grid added:", dev.id, dev.name, dev.serial)
end

--- static callback when any grid device is removed;
-- user scripts can redefine
-- @static
-- @param dev : a Grid table
function Grid.remove(dev) end

--- set grid rotation.
-- @tparam integer val : rotation 0,90,180,270 as [0, 3]
function Grid:rotation(val)
  _norns.grid_set_rotation(self.dev, val)
end


--- enable/disable grid tilt.
-- @tparam integer id : sensor
-- @tparam integer val : off/on [0, 1]
function Grid:tilt_enable(id, val)
  if (val == 1) then
    _norns.grid_tilt_enable(self.dev, id)
  else
    _norns.grid_tilt_disable(self.dev, id)
  end
end

--- set state of single LED on this grid device.
-- @tparam integer x : column index (1-based!)
-- @tparam integer y : row index (1-based!)
-- @tparam integer val : LED brightness in [0, 15]
function Grid:led(x, y, val)
  _norns.grid_set_led(self.dev, x, y, val)
end

--- set state of all LEDs on this grid device.
-- @tparam integer val : LED brightness in [0, 15]
function Grid:all(val)
  _norns.grid_all_led(self.dev, val)
end

--- update any dirty quads on this grid device.
function Grid:refresh()
  _norns.monome_refresh(self.dev)
end

--- intensity
function Grid:intensity(i)
  _norns.monome_intensity(self.dev, i)
end

--- create device, returns object with handler and send.
-- @static
-- @tparam integer n : vport index
function Grid.connect(n)
  local n = n or 1

  return Grid.vports[n]
end

--- clear handlers.
-- @static
function Grid.cleanup()
  for i=1,4 do
    Grid.vports[i].key = nil
    Grid.vports[i].tilt = nil
  end

  for _, dev in pairs(Grid.devices) do
    dev:all(0)
    dev:refresh()
    dev.key = nil
    dev.tilt = nil
  end

  Grid.add = function(dev)
    print("grid added:", dev.id, dev.name, dev.serial)
  end

  Grid.remove = function(dev) end
end

-- update devices.
-- @static
function Grid.update_devices()
  -- build list of available devices
  Grid.list = {}
  for _,device in pairs(Grid.devices) do
    device.port = nil
  end

  -- connect available devices to vports
  for i=1,4 do
    Grid.vports[i].device = nil
    Grid.vports[i].rows = 0
    Grid.vports[i].cols = 0       

    for _,device in pairs(Grid.devices) do
      if device.name == Grid.vports[i].name then
        Grid.vports[i].device = device
        Grid.vports[i].rows = device.rows
        Grid.vports[i].cols = device.cols
        device.port = i
      end
    end
  end
end

_norns.grid = {}

-- grid add
_norns.grid.add = function(id, serial, name, dev)
  local g = Grid.new(id,serial,name,dev)
  Grid.devices[id] = g
  Grid.update_devices()
  if Grid.add ~= nil then Grid.add(g) end
end

-- grid remove
_norns.grid.remove = function(id)
  local g = Grid.devices[id]
  if g then
    if Grid.vports[g.port].remove then
      Grid.vports[g.port].remove()
    end
    if Grid.remove then
      Grid.remove(Grid.devices[id])
    end
  end
  Grid.devices[id] = nil
  Grid.update_devices()
end

-- redefine global grid key input handler
_norns.grid.key = function(id, x, y, s)
  local g = Grid.devices[id]
  if g ~= nil then
    if g.key ~= nil then
      g.key(x, y, s)
    end

    if g.port then
      if Grid.vports[g.port].key then
        Grid.vports[g.port].key(x, y, s)
      end
    end
  else
    error('no entry for grid '..id)
  end
end

-- redefine global grid tilt input handler
_norns.grid.tilt = function(id, x, y, z)
  local g = Grid.devices[id]
  if g ~= nil then
    if g.tilt ~= nil then
      g.tilt(x, y, z)
    end

    if g.port then
      if Grid.vports[g.port].tilt then
        Grid.vports[g.port].tilt(x, y, z)
      end
    end
  else
    error('no entry for grid '..id)
  end
end


Grid.help = [[
--------------------------------------------------------------------------------
grid.connect( port )          create a grid table using device [port]
                                default [port] 1 if unspecified
                              (returns) grid table
.key( x, y, z )               function called with incoming grid key event
                                this should be redefined by the script
.tilt( x, y, z )               function called with incoming grid tilt event
                                this should be redefined by the script
.led( x, y, level )           set LED at [x,y] to [level]
                                [level] range is 0..15
.all( level )                 set all grid LED to [level]
                                [level] range is 0..15
.refresh()                    update the grid LED state

--------------------------------------------------------------------------------
-- example

lx,ly,lz = 0,0,0

-- connect grid
g = grid.connect()

-- key function
g.key = function(x,y,z)
  print(x,y,z)
  lx = x
  ly = y
  lz = z*15
  draw_grid()
end

-- simple draw function
draw_grid()
  g.all(0)
  g.led(lx,ly,lz)
  g.refresh()
end
--------------------------------------------------------------------------------
]]      

return Grid
