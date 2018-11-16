--- Arc class
-- @module arc
-- @alias Arc
require 'norns'


---------------------------------
-- Arc device class

local Arc = {}
Arc.devices = {}
Arc.list = {}
Arc.vport = {}
for i=1,4 do
  Arc.vport[i] = {
    name = "none",
    delta_callbacks = {},
    key_callbacks = {},
    index = 0,
    led = function() end,
    all = function() end,
    refresh = function() end,
    attached = false
  }
end
Arc.__index = Arc

--- constructor
-- @tparam integer id : arbitrary numeric identifier
-- @tparam string serial : serial
-- @tparam string name : name
-- @tparam userdata dev : opaque pointer to device
function Arc.new(id, serial, name, dev)
  local device = setmetatable({}, Arc)
  device.id = id
  device.serial = serial
  name = name .. " " .. serial
  --while tab.contains(Arc.list, name) do
  --  name = name .. "+"
  --end
  device.name = name
  device.dev = dev -- opaque pointer
  device.delta = nil -- delta event callback
  device.key = nil -- key event callback
  device.remove = nil -- device unplug callback
  device.ports = {} -- list of virtual ports this device is attached to

  -- autofill next postiion
  local connected = {}
  for i=1,4 do
    table.insert(connected, Arc.vport[i].name)
  end
  if not tab.contains(connected, name) then
    for i=1,4 do
      if Arc.vport[i].name == "none" then
        Arc.vport[i].name = name
        break
      end
    end
  end

  return device
end

--- static callback when any arc device is added;
-- user scripts can redefine
-- @param dev : a Arc table
function Arc.add(dev)
  print("arc added:", dev.id, dev.name, dev.serial)
end

--- scan device list and grab one, redefined later
function Arc.reconnect() end

--- static callback when any arc device is removed;
-- user scripts can redefine
-- @param dev : a Arc table
function Arc.remove(dev) end

--- set state of single LED on this arc device
-- @tparam integer ring : ring index (1-based!)
-- @tparam integer x : led index (1-based!)
-- @tparam integer val : LED brightness in [0, 15]
function Arc:led(ring, x, val)
  arc_set_led(self.dev, ring, x, val)
end

--- set state of all LEDs on this arc device
-- @tparam integer val : LED brightness in [0, 15]
function Arc:all(val)
  arc_all_led(self.dev, val)
end

--- update any dirty quads on this arc device
function Arc:refresh()
  monome_refresh(self.dev)
end

--- print a description of this arc device
function Arc:print()
  for k, v in pairs(self) do
    print('>> ', k, v)
  end
end

--- create device, returns object with handler and send
function Arc.connect(n)
  local n = n or 1
  if n > 4 then n = 4 end

  Arc.vport[n].index = Arc.vport[n].index + 1

  local d = {
    index = Arc.vport[n].index,
    port = n,
    delta = function(n, delta)
      print("arc input")
    end,
    key = function(n, s)
      print("arc input")
    end,
    attached = function() return Arc.vport[n].attached end,
    led = function(ring, x, val) Arc.vport[n].led(ring, x, val) end,
    all = function(val) Arc.vport[n].all(val) end,
    refresh = function() Arc.vport[n].refresh() end,
    disconnect = function(self)
      self.led = function() end
      self.all = function() end
      self.refresh = function() print("refresh: arc not connected") end
      Arc.vport[self.port].delta_callbacks[self.index] = nil
      Arc.vport[self.port].key_callbacks[self.index] = nil
      self.index = nil
      self.port = nil
    end,
    reconnect = function(self, p)
      p = p or 1
      if self.index then
        Arc.vport[self.port].delta_callbacks[self.index] = nil
        Arc.vport[self.port].key_callbacks[self.index] = nil
      end
      self.attached = function() return Arc.vport[p].attached end
      self.led = function(ring, x, val) Arc.vport[p].led(ring, x, val) end
      self.all = function(val) Arc.vport[p].all(val) end
      self.refresh = function() Arc.vport[p].refresh() end
      Arc.vport[p].index = Arc.vport[p].index + 1
      self.index = Arc.vport[p].index
      self.port = p
      Arc.vport[p].delta_callbacks[self.index] = function(n, delta) self.delta(n, delta) end
      Arc.vport[p].key_callbacks[self.index] = function(n, s) self.key(n, s) end
    end
  }

  Arc.vport[n].delta_callbacks[d.index] = function(n, delta) d.delta(n, delta) end
  Arc.vport[n].key_callbacks[d.index] = function(n, s) d.key(n, s) end

  return d
end

--- clear handlers
function Arc.cleanup()
  for i=1,4 do
    Arc.vport[i].delta_callbacks = {}
    Arc.vport[i].key_callbacks = {}
    Arc.vport[i].index = 0
  end
end

function Arc.update_devices()
  -- build list of available devices
  Arc.list = {}
  for _, device in pairs(Arc.devices) do
    table.insert(Arc.list, device.name)
    device.ports = {}
  end
  -- connect available devices to vports
  for i=1,4 do
    Arc.vport[i].attached = false
    Arc.vport[i].led = function(ring, x, val) end
    Arc.vport[i].all = function(val) end
    Arc.vport[i].refresh = function() end
    for _, device in pairs(Arc.devices) do
      if device.name == Arc.vport[i].name then
        Arc.vport[i].led = function(ring, x, val) device:led(ring, x, val) end
        Arc.vport[i].all = function(val) device:all(val) end
        Arc.vport[i].refresh = function() device:refresh() end
        Arc.vport[i].attached = true
        table.insert(device.ports, i)
      end
    end
  end
end

norns.arc = {}

-- arc devices
norns.arc.add = function(id, serial, name, dev)
  local g = Arc.new(id, serial, name, dev)
  Arc.devices[id] = g
  Arc.update_devices()
  if Arc.add ~= nil then Arc.add(g) end
end

norns.arc.remove = function(id)
  if Arc.devices[id] then
    if Arc.remove ~= nil then
      Arc.remove(Arc.devices[id])
    end
    if Arc.devices[id].remove then
      Arc.devices[id].remove()
    end
  end
  Arc.devices[id] = nil
  Arc.update_devices()
end

norns.arc.delta = function(id, n, delta)
  local device = Arc.devices[id]

  if device ~= nil then
    for _, i in pairs(device.ports) do
      for _, callback in pairs(Arc.vport[i].delta_callbacks) do
        callback(n, delta)
      end
    end
  else
    print('>> error: no entry for arc ' .. id)
  end
end

norns.arc.key = function(id, n, s)
  local device = Arc.devices[id]

  if device ~= nil then
    for _, i in pairs(device.ports) do
      for _, callback in pairs(Arc.vport[i].key_callbacks) do
        callback(n, s)
      end
    end
  else
    print('>> error: no entry for arc ' .. id)
  end
end

return Arc
