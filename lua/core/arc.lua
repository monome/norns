--- Arc class
-- @module arc
-- @alias Arc

---------------------------------
-- Arc device class

local Arc = {}
Arc.__index = Arc

Arc.devices = {}
Arc.list = {}
Arc.vports = {}

for i=1,4 do
  Arc.vports[i] = {
    name = "none",
    delta = function() end,
    key = function() end,

    led = function() end,
    all = function() end,
    refresh = function() end,
  }
end

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
  device.port = nil

  -- autofill next postiion
  local connected = {}
  for i=1,4 do
    table.insert(connected, Arc.vports[i].name)
  end
  if not tab.contains(connected, name) then
    for i=1,4 do
      if Arc.vports[i].name == "none" then
        Arc.vports[i].name = name
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

--- create device, returns object with handler and send
function Arc.connect(n)
  local n = n or 1

  return Arc.vports[n]
end

--- clear handlers
function Arc.cleanup()
  for i=1,4 do
    Arc.vports[i].delta = function() end
    Arc.vports[i].key = function() end
  end
end

function Arc.update_devices()
  -- build list of available devices
  Arc.list = {}
  for _, device in pairs(Arc.devices) do
    table.insert(Arc.list, device.name)
    device.port = nil
  end
  -- connect available devices to vports
  for i=1,4 do
    Arc.vports[i].led = function(ring, x, val) end
    Arc.vports[i].all = function(val) end
    Arc.vports[i].refresh = function() end
    for _, device in pairs(Arc.devices) do
      if device.name == Arc.vports[i].name then
        Arc.vports[i].led = function(ring, x, val) device:led(ring, x, val) end
        Arc.vports[i].all = function(val) device:all(val) end
        Arc.vports[i].refresh = function() device:refresh() end
        device.port = i
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
    if (device.port) then
      Arc.vports[device.port].delta(n, delta)
    end
  else
    error('no entry for arc '..id)
  end
end

norns.arc.key = function(id, n, s)
  local device = Arc.devices[id]

  if device ~= nil then
    if (device.port) then
      Arc.vports[device.port].key(n, s)
    end
  else
    error('no entry for arc '..id)
  end
end

return Arc
