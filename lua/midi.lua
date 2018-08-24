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
  name = name or 1
  local d = {
    event = function(data) print("midi input") end,
  }
  if name and type(name) == "number" then
    name = norns.state.ports.midi[name]
  end
  if not name or name == "all" then
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
  -- midi send helper functions
  d.note_on = function(note, vel, ch)
      d.send{type="note_on", note=note, vel=vel, ch=ch or 1}
    end
  d.note_off = function(note, vel, ch)
      d.send{type="note_off", note=note, vel=vel or 100, ch=ch or 1}
    end
  d.cc = function(cc, val, ch)
      d.send{type="cc", cc=cc, val=val, ch=ch or 1}
    end
  d.pitchbend = function(val, ch)
      d.send{type="pitchbend", val=val, ch=ch or 1}
    end
  d.aftertouch = function(note, val, ch)
      d.send{type="aftertouch", note=note, val=val, ch=ch or 1}
    end
  return d
end

--- clear handlers
function Midi.cleanup()
  Midi.broadcast = {}
  for name, t in pairs(Midi.callbacks) do
    Midi.callbacks[name] = {}
  end
end

-- utility

-- function table for msg-to-data conversion
local to_data = {
  -- FIXME: should all subfields have default values (ie note/vel?)
  note_on = function(msg)
      return {0x90 + (msg.ch or 1) - 1, msg.note, msg.vel or 100}
    end,
  note_off = function(msg)
      return {0x80 + (msg.ch or 1) - 1, msg.note, msg.vel or 100}
    end,
  cc = function(msg)
      return {0xb0 + (msg.ch or 1) - 1, msg.cc, msg.val}
    end,
  pitchbend = function(msg)
      return {0xe0 + (msg.ch or 1) - 1, msg.val & 0x7f, (msg.val >> 7) & 0x7f}
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
  local msg = {}
  -- note on
  if data[1] & 0xf0 == 0x90 then
    print("note")
    msg = {
      type = "note_on",
      note = data[2],
      vel = data[3],
      ch = data[1] - 0x90 + 1
    }
  -- note off
  elseif data[1] & 0xf0 == 0x80 then
    msg = {
      type = "note_off",
      note = data[2],
      vel = data[3],
      ch = data[1] - 0x80 + 1
    }
  -- cc
  elseif data[1] & 0xf0 == 0xb0 then
    msg = {
      type = "cc",
      cc = data[2],
      val = data[3],
      ch = data[1] - 0xb0 + 1
    }
  -- pitchbend
  elseif data[1] & 0xf0 == 0xe0 then
    msg = {
      type = "pitchbend",
      val = data[2] + (data[3] << 7),
      ch = data[1] - 0xe0 + 1
    }
  -- aftertouch
  elseif data[1] & 0xf0 == 0xa0 then
    msg = {
      type = "aftertouch",
      note = data[2],
      val = data[3],
      ch = data[1] - 0xa0 + 1
    }
  -- everything else
  else
    msg = {
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
    for _,device in pairs(Midi.callbacks[Midi.devices[id].name]) do
      device.event(data)
    end
    -- do broadcast callbacks
    for _,device in pairs(Midi.broadcast) do
      device.event(data)
    end
  end
  -- hack = send all midi to menu for param-cc-map
  norns.menu_midi_event(data)
end

return Midi
