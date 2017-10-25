--[[
   HID/other input device management
--]]

norns.version.input = '0.0.1'

-- input device class
Input = {}
Input.__index = Input

function Input:new(id, serial, name, types, codes)
   local d = setmetatable({}, Input)
   d.id = id
   d.serial = serial
   d.name = name
   d.types = types
   d.codes = {}
   -- `codes` is indexed by event type, values are subtables
   -- each subtable is indexed by supported code numbers, values are code names
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



-- global device management functions
norns.input = {}
norns.input.devices = {}
require('input_device_codes')
input = {} -- script callbacks go in here

norns.input.add = function(id, serial, name, types, codes)
   local d = Input:new(id, serial, name, types, codes)
   norns.input.devices[id] = d
   if input.add ~= nil then input.add(d) end
end

norns.input.remove = function()
   -- TODO!
end

norns.input.event = function(id, devtype, code, value)
   if input.event ~= nil then
      input.event(id, devtype, code, value)
   end
end
