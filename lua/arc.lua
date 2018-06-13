--- Arc class
-- @module arc
-- @alias Arc
require 'norns'
norns.version.arc = '0.0.2'


---------------------------------
-- Arc device class

local Arc = {}
Arc.devices = {}
Arc.__index = Arc

--- constructor
-- @tparam integer id : arbitrary numeric identifier
-- @tparam string serial : serial
-- @tparam string name : name
-- @tparam userdata dev : opaque pointer to device
function Arc.new(id, serial, name, dev)
  local a = setmetatable({}, Arc)
  a.id = id
  a.serial = serial
  a.name = name
  a.dev = dev -- opaque pointer
  a.enc = nil -- encoder event callback
  a.remove = nil -- device unplug callback
  a.encs = arc_encs(dev)
  return a
end

--- static callback when any arc device is added;
-- user scripts can redefine
-- @param dev : an Arc table
function Arc.add(dev)
  print("arc added", dev.id, dev.name)
end

--- scan device list and grab one, redefined later
function Arc.reconnect()
end

--- static callback when any arc device is removed;
-- user scripts can redefine
-- @param dev : an Arc table
function Arc.remove(dev)
  -- print("Arc.remove")
  -- dev:print()
end

--- set state of single LED on this arc device
-- @tparam integer enc : encoder index (1-based!)
-- @tparam integer led : led index (1-based!)
-- @tparam integer val : LED brightness in [1, 16]
function Arc:led(enc, led, val)
  arc_set_led(self.dev, enc, led, val)
end

--- set state of all LEDs on this arc device
-- @tparam integer val : LED brightness in [1, 16]
function Arc:all(val)
  arc_all_led(self.dev, val)
end

--- update any dirty quads on this arc device
function Arc:refresh()
  arc_refresh(self.dev)
end

--- print a description of this arc device
function Arc:print()
  for k,v in pairs(self) do
    print('>> ', k,v)
  end
end

-- arc devices
norns.arc.add = function(id, serial, name, dev)
  local a = Arc.new(id,serial,name,dev)
  Arc.devices[id] = a
  if Arc.add ~= nil then Arc.add(a) end
end

norns.arc.remove = function(id)
  if Arc.devices[id] then
    if Arc.remove ~= nil then
      Arc.remove(Arc.devices[id])
    end
  end
  Arc.devices[id] = nil
end

--- redefine global arc encoder input handler
norns.arc.enc = function(id, n, delta)
  local a = Arc.devices[id]
  if a ~= nil then
    if a.enc ~= nil then
      a.enc(n, delta)
    end
  else
    print('>> error: no entry for arc ')
  end
end

return Arc
