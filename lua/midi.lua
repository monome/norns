--- midi devices
-- @module midi
-- @alias Midi
require 'norns'

norns.version.midi = '0.0.0'

local tab = require 'tabutil'

local Midi = {}
Midi.devices = {}
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

--- send midi event to device
-- @param array
function Midi:send(data)
  midi_send(self.dev, data)
end

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
  end
  Midi.devices[id] = nil
end

--- handle a midi event
norns.midi.event = function(id, data)
  print("incoming midi message from device", string.format("%X", id))
  tab.print(data)

  local d = Midi.devices[id]
  if d ~= nil then
    if d.event ~= nil then
      d.event(data)
    end
  end
end

return Midi
