-- Number class
-- @module params.number

local Number = {}
Number.__index = Number

local tNUMBER = 1

function Number.new(id, name, min, max, default, formatter, wrap, allow_pmap)
  local o = setmetatable({}, Number)
  o.t = tNUMBER
  o.id = id
  o.name = name
  o.default = default or 0
  o.value = o.default
  o.min = min or -2147483648
  o.max = max or 2147483647 -- 32 bit signed
  o.range = math.abs(o.max - o.min) + 1
  o.formatter = formatter
  o.action = function() end
  o.wrap = wrap and o.range ~= 1 or false
  if allow_pmap == nil then o.allow_pmap = true else o.allow_pmap = allow_pmap end
  return o
end

function Number:get()
  return self.value
end

function Number:set(v, silent)
  local silent = silent or false
  if self.wrap then
    while v > self.max do
      v = v - self.range
    end
    while v < self.min do
      v = v + self.range
    end
  end
  local c = util.clamp(v,self.min,self.max)
  if self.value ~= c then
    self.value = c
    if silent==false then self:bang() end
  end
  if norns.pmap.data[self.id] ~= nil then
    local midi_prm = norns.pmap.data[self.id]
    midi_prm.value = util.round(util.linlin(midi_prm.out_lo, midi_prm.out_hi, midi_prm.in_lo, midi_prm.in_hi, self.value))
    if midi_prm.echo then
      local port = norns.pmap.data[self.id].dev
      midi.vports[port]:cc(midi_prm.cc, midi_prm.value, midi_prm.ch)
    end
  end
end

function Number:delta(d)
  self:set(self:get() + d)
end

function Number:set_default()
  self:set(self.default)
end

function Number:bang()
  self.action(self.value)
end

function Number:string()
  if self.formatter then
    return self.formatter(self)
  else
    return self.value
  end
end

function Number:get_range()
  local r = { self.min, self.max }
  return r
end


return Number
