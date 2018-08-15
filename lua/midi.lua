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
function Midi.add(dev) end

--- static callback when any midi device is removed;
-- user scripts can redefine
-- @param dev : a Midi table
function Midi.remove(dev) end

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
  if data.type then
    print("msg")
    local d = Midi.to_data(data)
    if d then midi_send(self.dev, d) end
  else
    print("data")
    midi_send(self.dev, data)
  end
end

--- send midi event to named device
function Midi.send_named(name, data)
  if Midi.reverse[name] then
    Midi.devices[Midi.reverse[name]]:send(data)
  end
end

--- send midi event to all devices
function Midi.send_all(data)
  for _,device in pairs(Midi.devices) do
    device:send(data)
  end
end


--- create device, returns object with handler and send
function Midi.connect(name)
  local d = {
    handler = function(data) print("midi input") end,
  }
  if not name then
    d.send = function(data) midi.send_all(data) end
    table.insert(Midi.broadcast, d)
  else
    if not Midi.callbacks[name] then
      Midi.callbacks[name] = {}
      Midi.reverse[name] = nil -- will be overwritten with device insertion
    end
    d.send = function(data) midi.send_named(name, data) end
    table.insert(Midi.callbacks[name], d)
  end
  return d
end

--- clear handlers
function Midi.clear()
  Midi.broadcast = {}
  for name, t in pairs(Midi.callbacks) do
    Midi.callbacks[name] = {}
  end
end

-- utility

-- function table for msg-to-data conversion
local to_data = {
  -- FIXME: should all subfields have default values (ie note/vel?)
  note = function(msg)
    return {0x90 + (msg.ch or 1) - 1, msg.note, msg.vel}
    end,
  cc = function(msg)
    return {0xb0 + (msg.ch or 1) - 1, msg.cc, msg.val}
    end,
  aftertouch = function(msg)
    return {0xa0 + (msg.ch or 1) - 1, msg.note, msg.val}
    end
}

--- convert msg to data (midi bytes)
function Midi.to_data(msg)
  if msg.type then
    return to_data[msg.type](msg)
  else return nil end
end

--- convert data (midi bytes) to msg
function Midi.to_msg(data)
  -- FIXME these comparisons might be faster with bitops in lua?
  -- note on
  if data[1] >= 0x90 and data[1] <= 0x90 + 15 then
    local msg = {
      type = "note",
      note = data[2],
      vel = data[3],
      ch = data[1] - 0x90 + 1
    }
  -- note off
  elseif data[1] >= 0x80 and data[1] <= 0x80 + 15 then
    local msg = {
      type = "note",
      note = data[2],
      --vel = data[3],
      -- FIXME (maybe?) ignoring note-off velocity
      -- otherwise we can create two types: note_on and note_off
      vel = 0,
      ch = data[1] - 0x80 + 1
    }
  -- cc
  elseif data[1] >= 0xb0 and data[1] <= 0xb0 + 15 then
    local msg = {
      type = "cc",
      cc = data[2],
      val = data[3],
      ch = data[1] - 0xb0 + 1
    }
  -- aftertouch
  elseif data[1] >= 0xa0 and data[1] <= 0xa0 + 15 then
    local msg = {
      type = "aftertouch",
      note = data[2],
      val = data[3],
      ch = data[1] - 0xa0 + 1
    }
  -- everything else
  else
    local msg = {
      type = "other"
    }
  end
  return msg
end



--- norns functions

--- add a device
norns.midi.add = function(id, name, dev)
  print("midi added:", id, name)
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
    for _,device in pairs(Midi.devices[id].callbacks) do
      device.handler(data)
    end
    -- do broadcast callbacks
    for _,device in pairs(Midi.broadcast) do
      device.handler(data)
    end
  end
  -- hack = send all midi to menu for param-cc-map
  norns.menu_midi_event(data)
end

return Midi
