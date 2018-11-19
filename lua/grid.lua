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
    rotation = function() end,
    refresh = function() end,
    cols = 0,
    rows = 0,
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
  --  name = name .. "+"
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
  for i=1,4 do
    table.insert(connected, Grid.vport[i].name)
  end
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

-- set grid rotation
-- @tparam integer val : rotation 0,90,180,270 as [0, 3]
function Grid:rotation(val)
  grid_set_rotation(self.dev, val)
end

--- set state of single LED on this grid device
-- @tparam integer x : column index (1-based!)
-- @tparam integer y : row index (1-based!)
-- @tparam integer val : LED brightness in [0, 15]
function Grid:led(x, y, val)
  grid_set_led(self.dev, x, y, val)
end

--- set state of all LEDs on this grid device
-- @tparam integer val : LED brightness in [0, 15]
function Grid:all(val)
  grid_all_led(self.dev, val)
end

--- update any dirty quads on this grid device
function Grid:refresh()
  monome_refresh(self.dev)
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
    cols = function() return Grid.vport[n].cols end,
    rows = function() return Grid.vport[n].rows end,
    event = function(x,y,z)
        print("grid input")
      end,
    attached = function() return Grid.vport[n].attached end,
    led = function(x,y,z) Grid.vport[n].led(x,y,z) end,
    all = function(val) Grid.vport[n].all(val) end,
    rotation = function(val) Grid.vport[n].rotation(val) end,
    refresh = function() Grid.vport[n].refresh() end,
    disconnect = function(self)
        self.led = function() end
        self.all = function() end
        self.rotation = function() end
        self.refresh = function() print("refresh: grid not connected") end
        Grid.vport[self.port].callbacks[self.index] = nil
        self.index = nil
        self.port = nil
      end,
    reconnect = function(self, p)
        p = p or 1
        if self.index then
          Grid.vport[self.port].callbacks[self.index] = nil
        end
        self.attached = function() return Grid.vport[p].attached end
        self.led = function(x,y,z) Grid.vport[p].led(x,y,z) end
        self.all = function(val) Grid.vport[p].all(val) end
        self.rotation = function(val) Grid.vport[p].rotation(val) end
        self.refresh = function() Grid.vport[p].refresh() end
        Grid.vport[p].index = Grid.vport[p].index + 1
        self.index = Grid.vport[p].index
        self.port = p
        self.cols = function() return Grid.vport[p].cols end
        self.rows = function() return Grid.vport[p].rows end
        Grid.vport[p].callbacks[self.index] = function(x,y,z) self.event(x,y,z) end
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
    Grid.vport[i].rotation = function(val) end
    Grid.vport[i].refresh = function() end
    Grid.vport[i].cols = 0
    Grid.vport[i].rows = 0
    for _,device in pairs(Grid.devices) do
      if device.name == Grid.vport[i].name then
        Grid.vport[i].cols = device.cols
        Grid.vport[i].rows = device.rows
        Grid.vport[i].led = function(x, y, val) device:led(x, y, val) end
        Grid.vport[i].all = function(val) device:all(val) end
        Grid.vport[i].rotation = function(val) device:rotation(val) end
        Grid.vport[i].refresh = function() device:refresh() end
        Grid.vport[i].attached = true
        table.insert(device.ports, i)
      end
    end
  end
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
