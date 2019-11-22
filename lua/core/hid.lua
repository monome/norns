--- Hid class
-- @module hid
-- @alias Hid

---------------------------------
-- Hid device class

local vport = require 'vport'
local hid_events = require 'hid_events'

local Hid = {}
Hid.__index = Hid

Hid.types = hid_events.types
Hid.codes = hid_events.codes

Hid.devices = {}
Hid.vports = {}

for i=1,4 do
  Hid.vports[i] = {
    name = "none",
    device = nil,

    event = nil,
  }
end

--- constructor
-- @tparam integer id : arbitrary numeric identifier
-- @tparam string name : name
-- @tparam string types : array of supported event types. keys are type codes, values are strings
-- @tparam userdata codes : array of supported codes. each entry is a table of codes of a given type. subtables are indexed by supported code numbers; values are code names
-- @tparam userdata dev : opaque pointer to device
function Hid.new(id, name, types, codes, dev)
  local device = setmetatable({}, Hid)

  device.id = id
  device.name = vport.get_unique_device_name(name, Hid.devices)
  device.dev = dev -- opaque pointer
  device.event = nil -- event callback
  device.remove = nil -- device unplug callback
  device.port = nil

  -- autofill next postiion
  local connected = {}
  for i=1,4 do
    table.insert(connected, Hid.vports[i].name)
  end
  if not tab.contains(connected, device.name) then
    for i=1,4 do
      if Hid.vports[i].name == "none" then
        Hid.vports[i].name = device.name
        break
      end
    end
  end

  return device
end

--- static callback when any hid device is added;
-- user scripts can redefine
-- @param dev : a Hid table
function Hid.add(dev)
  print("hid added:", dev.id, dev.name)
end

--- static callback when any hid device is removed;
-- user scripts can redefine
-- @param dev : a Hid table
function Hid.remove(dev) end


--- create device, returns object with handler and send
function Hid.connect(n)
  local n = n or 1

  return Hid.vports[n]
end

--- clear handlers
function Hid.cleanup()
  for i=1,4 do
    Hid.vports[i].event = nil
  end

  for _, dev in pairs(Hid.devices) do
    dev.event = nil
  end
end

function Hid.update_devices()
  -- reset vports for existing devices
  for _, device in pairs(Hid.devices) do
    device.port = nil
  end

  -- connect available devices to vports
  for i=1,4 do
    Hid.vports[i].device = nil

    for _, device in pairs(Hid.devices) do
      if device.name == Hid.vports[i].name then
        Hid.vports[i].device = device
        device.port = i
      end
    end
  end
end

_norns.hid = {}

-- hid devices
_norns.hid.add = function(id, name, types, codes, dev)
  local g = Hid.new(id, name, types, codes, dev)
  Hid.devices[id] = g
  Hid.update_devices()
  if Hid.add ~= nil then Hid.add(g) end
end

_norns.hid.remove = function(id)
  if Hid.devices[id] then
    if Hid.remove ~= nil then
      Hid.remove(Hid.devices[id])
    end
    if Hid.devices[id].remove then
      Hid.devices[id].remove()
    end
  end
  Hid.devices[id] = nil
  Hid.update_devices()
end

_norns.hid.event = function(id, type, code, value)
  local device = Hid.devices[id]

  if device ~= nil then
    if device.event then
      device.event(type, code, value)
    end

    if device.port then
      if Hid.vports[device.port].event then
        Hid.vports[device.port].event(type, code, value)
      end
    end
  else
    error('no entry for hid '..id)
  end
end

return Hid
