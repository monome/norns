--- midi devices
-- @module midi
-- @alias Midi

local vport = require 'vport'

local Midi = {}
Midi.__index = Midi

Midi.devices = {}
Midi.vports = {}

for i=1,4 do
  Midi.vports[i] = {
    name = "none",
    device = nil,
    event = nil,

    send = function(self, ...) if self.device then self.device:send(...) end end,

    note_on = vport.wrap_method('note_on'),
    note_off = vport.wrap_method('note_off'),
    cc = vport.wrap_method('cc'),
    pitchbend = vport.wrap_method('pitchbend'),
    key_pressure = vport.wrap_method('key_pressure'),
    channel_pressure = vport.wrap_method('channel_pressure'),
    program_change = vport.wrap_method('program_change'),
    start = vport.wrap_method('start'),
    stop = vport.wrap_method('stop'),
    continue = vport.wrap_method('continue'),
    clock = vport.wrap_method('clock'),
    song_position = vport.wrap_method('song_position'),
    song_select = vport.wrap_method('song_select'),
  }
end

--- constructor
-- @tparam integer id : arbitrary numeric identifier
-- @tparam string name : name
-- @tparam userdata dev : opaque pointer to device
function Midi.new(id, name, dev)
  local d = setmetatable({}, Midi)

  d.id = id
  d.name = vport.get_unique_device_name(name, Midi.devices)
  d.dev = dev -- opaque pointer
  d.event = nil -- event callback
  d.remove = nil -- device unplug callback
  d.port = nil

  -- autofill next postiion
  local connected = {}
  for i=1,4 do
    table.insert(connected, Midi.vports[i].name)
  end
  if not tab.contains(connected, name) then
    for i=1,4 do
      if Midi.vports[i].name == "none" then
        Midi.vports[i].name = d.name
        break
      end
    end
  end

  return d
end

--- static callback when any midi device is added.
-- user scripts can redefine.
-- @param dev : a Midi table
function Midi.add(dev) end

--- static callback when any midi device is removed.
-- user scripts can redefine.
-- @param dev : a Midi table
function Midi.remove(dev) end

--- send midi event to device.
-- @param data
function Midi:send(data)
  if data.type then
    local d = Midi.to_data(data)
    _norns.midi_send(self.dev, d)
  else
    _norns.midi_send(self.dev, data)
  end
end

--- send midi note on event.
-- @tparam integer note : note number
-- @tparam integer vel : velocity 
-- @tparam integer ch : midi channel
function Midi:note_on(note, vel, ch)
  self:send{type="note_on", note=note, vel=vel, ch=ch or 1}
end

--- send midi note off event.
-- @tparam integer note : note number
-- @tparam integer vel : velocity 
-- @tparam integer ch : midi channel
function Midi:note_off(note, vel, ch)
  self:send{type="note_off", note=note, vel=vel or 100, ch=ch or 1}
end

--- send midi continuous controller event.
-- @tparam integer cc : cc number
-- @tparam integer val : value 
-- @tparam integer ch : midi channel
function Midi:cc(cc, val, ch)
  self:send{type="cc", cc=cc, val=val, ch=ch or 1}
end

--- send midi pitchbend event.
-- @tparam integer val : value 
-- @tparam integer ch : midi channel
function Midi:pitchbend(val, ch)
  self:send{type="pitchbend", val=val, ch=ch or 1}
end

--- send midi key pressure event.
-- @tparam integer note : note number
-- @tparam integer val : value 
-- @tparam integer ch : midi channel
function Midi:key_pressure(note, val, ch)
  self:send{type="key_pressure", note=note, val=val, ch=ch or 1}
end

--- send midi channel pressure event.
-- @tparam integer val : value 
-- @tparam integer ch : midi channel
function Midi:channel_pressure(val, ch)
  self:send{type="channel_pressure", val=val, ch=ch or 1}
end

--- send midi program change event.
-- @tparam integer val : value 
-- @tparam integer ch : midi channel
function Midi:program_change(val, ch)
  self:send{type="program_change", val=val, ch=ch or 1}
end

--- send midi start event.
function Midi:start()
  self:send{type="start"}
end

--- send midi stop event.
function Midi:stop()
  self:send{type="stop"}
end

--- send midi continue event.
function Midi:continue()
  self:send{type="continue"}
end

--- send midi clock event.
function Midi:clock()
  self:send{type="clock"}
end

--- send midi song position event.
-- @tparam integer lsb :  
-- @tparam integer msb : 
function Midi:song_position(lsb, msb)
  self:send{type="song_position", lsb=lsb, msb=msb}
end

--- send midi song select event.
-- @tparam integer val : value
function Midi:song_select(val)
  self:send{type="song_select", val=val}
end

--- create device, returns object with handler and send.
-- @tparam integer n : vport index
function Midi.connect(n)
  local n = n or 1
  return Midi.vports[n]
end

--- clear handlers.
function Midi.cleanup()
  for i=1,4 do
    Midi.vports[i].event = nil
  end

  for _, dev in pairs(Midi.devices) do
    dev.event = nil
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
  key_pressure = function(msg)
      return {0xa0 + (msg.ch or 1) - 1, msg.note, msg.val}
    end,
  channel_pressure = function(msg)
      return {0xd0 + (msg.ch or 1) - 1, msg.val}
    end,
  program_change = function(msg)
      return {0xc0 + (msg.ch or 1) - 1, msg.val}
    end,
  start = function(msg)
      return {0xfa}
    end,
  stop = function(msg)
      return {0xfc}
    end,
  continue = function(msg)
      return {0xfb}
    end,
  clock = function(msg)
      return {0xf8}
    end,
  song_position = function(msg)
      return {0xf2, msg.lsb, msg.msb}
    end,
  song_select = function(msg)
      return {0xf3, msg.val}
    end
}

--- convert msg to data (midi bytes).
-- @tparam table msg : 
-- @treturn table data : table of midi status and data bytes
function Midi.to_data(msg)
  if msg.type then
    return to_data[msg.type](msg)
  else
    error('failed to serialize midi message')
  end
end

--- convert data (midi bytes) to msg.
-- @tparam table data :
-- @treturn table msg : midi message table, contents vary depending on message
function Midi.to_msg(data)
  local msg = {}
  -- note on
  if data[1] & 0xf0 == 0x90 then
    msg = {
      note = data[2],
      vel = data[3],
      ch = data[1] - 0x90 + 1
    }
    if data[3] > 0 then
      msg.type = "note_on"
    elseif data[3] == 0 then -- if velocity is zero then send note off
      msg.type = "note_off"
    end
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
  -- key pressure
  elseif data[1] & 0xf0 == 0xa0 then
    msg = {
      type = "key_pressure",
      note = data[2],
      val = data[3],
      ch = data[1] - 0xa0 + 1
    }
  -- channel pressure
  elseif data[1] & 0xf0 == 0xd0 then
    msg = {
      type = "channel_pressure",
      val = data[2],
      ch = data[1] - 0xd0 + 1
    }
  -- program change
  elseif data[1] & 0xf0 == 0xc0 then
    msg = {
      type = "program_change",
      val = data[2],
      ch = data[1] - 0xc0 + 1
    }
  -- start
  elseif data[1] == 0xfa then
    msg.type = "start"
  -- stop
  elseif data[1] == 0xfc then
     msg.type = "stop"
  -- continue
  elseif data[1] == 0xfb then
    msg.type = "continue"
  -- clock
  elseif data[1] == 0xf8 then
    msg.type = "clock"
  -- song position pointer
  elseif data[1] == 0xf2 then
    msg = {
        type = "song_position",
        lsb = data[2],
        msb = data[3]
    }
  -- song select
  elseif data[1] == 0xf3 then
    msg = {
        type = "song_select",
        val = data[2]
    }
  -- active sensing (should probably ignore)
  elseif data[1] == 0xfe then
      -- do nothing
  -- everything else
  else
    msg = {
      type = "other",
    }
  end
  return msg
end

--- update devices.
function Midi.update_devices()
  -- reset vports for existing devices
  for _,device in pairs(Midi.devices) do
    device.port = nil
  end

  -- connect available devices to vports
  for i=1,4 do
    Midi.vports[i].device = nil

    for _, device in pairs(Midi.devices) do
      if device.name == Midi.vports[i].name then
        Midi.vports[i].device = device
        device.port = i
      end
    end
  end
end

--- add a device.
_norns.midi.add = function(id, name, dev)
  local d = Midi.new(id, name, dev)
  Midi.devices[id] = d
  Midi.update_devices()
  if Midi.add ~= nil then Midi.add(d) end
end

--- remove a device.
_norns.midi.remove = function(id)
  if Midi.devices[id] then
    if Midi.devices[id].remove then
      Midi.devices[id].remove()
    end
  end
  Midi.devices[id] = nil
  Midi.update_devices()
end

--- handle a midi event.
_norns.midi.event = function(id, data)
  local d = Midi.devices[id]

  if d ~= nil then
    if d.event ~= nil then
      d.event(data)
    end

    if d.port and Midi.vports[d.port].event then
      Midi.vports[d.port].event(data)
    end
  else
    error('no entry for midi '..id)
  end

  -- hack = send all midi to menu for param-cc-map
  norns.menu_midi_event(data)
end

return Midi
