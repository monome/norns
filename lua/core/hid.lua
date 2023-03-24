--- Hid class
-- @module hid

local vport = require 'vport'
local hid_events = require 'hid_events'
local hid_device_class = require 'hid_device_class'
local tab  = require 'tabutil'

local gamepad  = require 'gamepad'

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

-- constructor
-- @tparam integer id : arbitrary numeric identifier
-- @tparam string name : name
-- @tparam table types : array of supported event types. keys are type codes, values are strings
-- @tparam table codes : array of supported codes. each entry is a table of codes of a given type. subtables are indexed by supported code numbers; values are code names
-- @tparam userdata dev : opaque pointer to device
function Hid.new(id, name, types, codes, dev, guid)
  local device = setmetatable({}, Hid)

  device.id = id
  device.name = vport.get_unique_device_name(name, Hid.devices)
  device.dev = dev -- opaque pointer
  device.guid = guid -- SDL format GUID
  device.event = nil -- event callback
  device.remove = nil -- device unplug callback
  device.port = nil

  -- copy the types and codes tables
  device.types = {}
  device.codes = {}
  -- types table shall be a simple array with default indexing
  for k,v in pairs(types) do
    device.types[k] = v
  end
  -- codes table shall be an associate array indexed by type
  for k,v in pairs(codes) do
    device.codes[types[k]] = {}
    for kk,vv in pairs(v) do
      device.codes[types[k]][kk] = vv
    end
  end

  device.is_ascii_keyboard = hid_device_class.is_ascii_keyboard(device)
  device.is_mouse = hid_device_class.is_mouse(device)
  device.is_gamepad = hid_device_class.is_gamepad(device)

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
-- @static
-- @param dev : a Hid table
function Hid.add(dev)
  print("HID device was added:", dev.id, dev.name, dev.guid)
  if dev.is_ascii_keyboard then print("this appears to be an ASCII keyboard!") end
  if dev.is_mouse then print("this appears to be a mouse!") end
  if dev.is_gamepad then print("this appears to be a gamepad!") end
end

--- static callback when any hid device is removed;
-- user scripts can redefine
-- @static
-- @param dev : a Hid table
function Hid.remove(dev) end

--- create device, returns object with handler and send
-- @static
function Hid.connect(n)
  local n = n or 1

  return Hid.vports[n]
end

-- clear handlers
-- @static
function Hid.cleanup()
  for i=1,4 do
    Hid.vports[i].event = nil
  end

  for _, dev in pairs(Hid.devices) do
    dev.event = nil
  end

  Hid.add = function(dev)
    print("HID device was added:", dev.id, dev.name)
    if dev.is_ascii_keyboard then print("this appears to be an ASCII keyboard!") end
    if dev.is_mouse then print("this appears to be a mouse!") end
    if dev.is_gamepad then print("this appears to be a gamepad!") end
  end

  Hid.remove = function(dev) end
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
_norns.hid.add = function(id, name, types, codes, dev, guid)
  local g = Hid.new(id, name, types, codes, dev, guid)
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

    if device.is_ascii_keyboard then
      keyboard.process(type, code, value)
    elseif device.is_gamepad then
      gamepad.process(device.guid, type, code, value)
    end
  else
    error('no entry for hid '..id)
  end
end

return Hid
