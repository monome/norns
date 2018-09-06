--- midi devices
-- @module midi
-- @alias Midi
require 'norns'

local Midi = {}
Midi.devices = {}
Midi.list = {}
Midi.vport = {}
for i=1,4 do
  Midi.vport[i] = {
    name = "none",
    callbacks = {},
    index = 0,
    send = function() end,
    attached = false
  }
end
Midi.__index = Midi

--- constructor
-- @tparam integer id : arbitrary numeric identifier
-- @tparam string name : name
-- @tparam userdata dev : opaque pointer to device
function Midi.new(id, name, dev)
  local d = setmetatable({}, Midi)
  d.id = id
  -- append duplicate device names
  --while tab.contains(Midi.list,name) do
    --name = name .. "+"
  --end
  d.name = name
  d.dev = dev -- opaque pointer
  d.event = nil -- event callback
  d.remove = nil -- device unplug callback
  d.ports = {} -- list of virtual ports this device is attached to

  -- autofill next postiion
  local connected = {}
  for i=1,4 do table.insert(connected, Midi.vport[i].name) end
  if not tab.contains(connected, name) then
    for i=1,4 do
      if Midi.vport[i].name == "none" then
        Midi.vport[i].name = name
        break
      end
    end
  end

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
    --print("msg")
    local d = Midi.to_data(data)
    if d then midi_send(self.dev, d) end
  else
    --print("data")
    midi_send(self.dev, data)
  end
end


--- create device, returns object with handler and send
function Midi.connect(n)
  local n = n or 1
  if n>4 then n=4 end

  Midi.vport[n].index = Midi.vport[n].index + 1

  local d = {
    index = Midi.vport[n].index,
    port = n,
    event = function(data)
        print("midi input")
      end,
    attached = function() return Midi.vport[n].attached end,
    send = function(data) Midi.vport[n].send(data) end,
    disconnect = function(self)
        self.send = function() print("not connected") end
        table.remove(Midi.vport[self.port].callbacks, self.index)
        self.index = nil
        self.port = nil
      end,
    reconnect = function(self, p)
        if self.index then
          table.remove(Midi.vport[self.port].callbacks, self.index)
        end
        self.send = function(data) Midi.vport[p].send(data) end
        attached = function() return Midi.vport[p].attached end
        Midi.vport[p].index = Midi.vport[p].index + 1
        self.index = Midi.vport[p].index
        self.port = p
        Midi.vport[p].callbacks[self.index] = function(data) self.event(data) end
      end
  }

  Midi.vport[n].callbacks[d.index] = function(data) d.event(data) end

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
  for i=1,4 do
    Midi.vport[i].callbacks = {}
    Midi.vport[i].index = 0
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
    --print("note")
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


function Midi.update_devices()
  -- build list of available devices
  Midi.list = {}
  for _,device in pairs(Midi.devices) do
    table.insert(Midi.list, device.name)
    device.ports = {}
  end
  -- connect available devices to vports
  for i=1,4 do
    Midi.vport[i].attached = false
    Midi.vport[i].send = function(data) end
    for _,device in pairs(Midi.devices) do
      if device.name == Midi.vport[i].name then
        Midi.vport[i].send = function(data) device:send(data) end
        Midi.vport[i].attached = true
        table.insert(device.ports, i)
      end
    end
  end
end

--- norns functions

--- add a device
norns.midi.add = function(id, name, dev)
  print("midi added:", name)
  local d = Midi.new(id, name, dev)
  Midi.devices[id] = d
  Midi.update_devices()
  if Midi.add ~= nil then Midi.add(d) end
end

--- remove a device
norns.midi.remove = function(id)
  if Midi.devices[id] then
  print("midi removed:", Midi.devices[id].name)
    if Midi.devices[id].remove then
      Midi.devices[id].remove()
    end
  end
  Midi.devices[id] = nil
  Midi.update_devices()
end

--- handle a midi event
norns.midi.event = function(id, data)
  -- iterate through port table

  local d = Midi.devices[id]
  if d ~= nil then
    if d.event ~= nil then
      d.event(data)
    end

    for _,n in pairs(d.ports) do
      for _,event in pairs(Midi.vport[n].callbacks) do
        --print("vport " .. n)
        event(data)
      end
    end
  end
  -- hack = send all midi to menu for param-cc-map
  norns.menu_midi_event(data)
end

return Midi
