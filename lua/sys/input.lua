-- require 'norns'

norns.input = {}
norns.input.devices = {}

require('input_device_codes')

-- input device class
Input = {}
Input.__index = Input

--- constructor
-- `codes` is indexed by event type, values are subtables
-- each subtable is indexed by supported code numbers; values are code names
-- @param id : arbitrary numberical index of the device
-- @param serial : serial device string from USB
-- @param name: device name string fromUSB
-- @param types: array of supported event types
-- @param codes: array of supported codes. this is 
function Input.new(id, serial, name, types, codes)
   local d = setmetatable({}, Input)
   -- print(id, serial, name, types, codes)
   d.id = id
   d.serial = serial
   d.name = name
   d.types = types
   d.codes = {}
   for i,t in pairs(types) do
      if norns.input.event_types[t] ~= nil then
	 d.codes[t] = {}
	 for j,c in pairs(codes[i]) do
	    d.codes[t][c] = norns.input.event_codes[t][c]
	 end
      end
   end
   return d
end

function Input:print()
   print(self.id, self.serial, self.name)
   print('event types: ')
   for t,arr in pairs(self.codes) do
      if norns.input.event_types[t] ~= nil then
	 print(t, norns.input.event_types[t])
      else
	 print(t, '(unsupported)')
      end
      for id,name in pairs(arr) do
	 print('', id, name)
      end
   end
end


norns.input.add = function(id, serial, name, types, codes)
   -- print(id, serial, name, types, codes)
   local d = Input.new(id, serial, name, types, codes)
   norns.input.devices[id] = d

   d:print()

   -- FIXME: not good; encapsulate
   -- if input.add ~= nil then input.add(d) end

end

norns.input.remove = function()
   -- TODO!
end

norns.input.event = function(id, devtype, code, value)
   print(id, devtype, code, value)
   --- FIXME: perform registered callback(s?) for device
   --[[
   if input.event ~= nil then
      input.event(id, devtype, code, value)
   end
   --]]
end
