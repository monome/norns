--- midi devices
-- @module midi
-- @alias Midi
require 'norns'

norns.version.midi = '0.0.0'

local Midi = {}
Midi.devices = {}
Midi.callbacks = {} -- tables per device, with subscription callbacks
Midi.broadcast = {} -- table with callbacks for "all devices"
Midi.reverse = {} -- reverse lookup, names/id
Midi.__index = Midi

--- constructor
-- @tparam integer id : arbitrary numeric identifier
-- @tparam string name : name
-- @tparam userdata dev : opaque pointer to device
function Midi.new(id, name, dev)
  local d = setmetatable({}, Midi)
  d.id = id
  d.name = name
  d.dev = dev -- opaque pointer
  d.event = nil -- event callback
  d.remove = nil -- device unplug callback 
  -- update callback table
  if not Midi.callbacks[name] then
    Midi.callbacks[name] = {}
  end
  d.callbacks = Midi.callbacks[name] 
  -- update reverse lookup
  Midi.reverse[name] = id 
  return d
end

--- static callback when any midi device is added;
-- user scripts can redefine
-- @param dev : a Midi table
function Midi.add(dev)
  print("midi added:", dev.id, dev.name)
end

--- static callback when any midi device is removed;
-- user scripts can redefine
-- @param dev : a Midi table
function Midi.remove(dev)
  print("midi removed:", dev.id, dev.name)
end

--- call add() for currently available devices
-- when scripts are restarted
function Midi.reconnect()
  for id,dev in pairs(Midi.devices) do
    if Midi.add ~= nil then Midi.add(dev) end
  end
end

--- send midi event to device
-- @param array
function Midi:send(data)
  midi_send(self.dev, data)
end

--- send midi event to named device
function Midi:send_named(name, data)
  if Midi.reverse[name] then
    midi_send(Midi.devices[Midi.reverse[name]],data)
  end
end

--- send midi event to all devices
function Midi.send_all(data)
  for id,device in pairs(Midi.devices) do
    midi_send(device.dev, data)
  end
end


--- subscribe
function Midi.subscribe(callback, name)
  if not name then
    table.insert(Midi.broadcast, callback)
    return function(data) midi.send_all(data) end
  else
    if not Midi.callbacks[name] then
      Midi.callbacks[name] = {}
      Midi.reverse[name] = nil -- will be overwritten with device insertion
    end
    table.insert(Midi.callbacks[name], callback)
    return function(data) midi.send_name(name, data) end
  end
end

--- unsubscribe
function Midi.unsubscribe(callback)
  for i,v in pairs(Midi.broadcast) do
    if v == callback then
      Midi.broadcast[i] = nil
    end
  end
  for name, t in pairs(Midi.callbacks) do
    for i,v in pairs(t) do
      if v == callback then
        Midi.callbacks[name][i] = nil
      end
    end
  end
end

--- clear subscriptions
function Midi.clear()
  Midi.broadcast = {}
  for name, t in pairs(Midi.callbacks) do
    Midi.callbacks[name] = {}
  end
end



--- norns functions

--- add a device
norns.midi.add = function(id, name, dev)
  local d = Midi.new(id, name, dev)
  Midi.devices[id] = d
  if Midi.add ~= nil then Midi.add(d) end
end

--- remove a device
norns.midi.remove = function(id)
  if Midi.devices[id] then
    if Midi.remove ~= nil then
      Midi.remove(Midi.devices[id])
    end
    if Midi.devices[id].remove then
      Midi.devices[id].remove()
    end
  end
  Midi.devices[id] = nil
end

--- handle a midi event
norns.midi.event = function(id, data)
  local d = Midi.devices[id]
  if d ~= nil then
    if d.event ~= nil then
      d.event(data)
    end
    -- do any individual subscribed callbacks
    for name,callback in pairs(Midi.devices[id].callbacks) do
      callback(data)
    end
    -- do broadcast callbacks
    for n,callback in pairs(Midi.broadcast) do
      callback(data)
    end
  end
  -- hack = send all midi to menu for param-cc-map
  norns.menu_midi_event(data)
end

return Midi
