

InputDevice = {}
InputDevice.__index = InputDevice

function InputDevice:new(id, serial, name, types, codes)
   local d = setmetatable({}, InputDevice)
   d.id = id
   d.serial = serial
   d.name = name
   -- `codes` is indexed by event type, values are subtables
   -- each subtable is indexed by supported code numbers, values are code names
   d.codes = {}
   for i,t in pairs(types) do
	  if input.event_types[v] ~= nil then
		 d.codes[t] = {}
		 for _,c in pairs(codes[i]) do
			d.codes[t][c] = input.event_codes[t][c]
		 end
	  end
   end
   d.types = types
   d.codes = codes
   return d
end

function InputDevice:print()
   print(self.id, self.serial, self.name)
   print('>> event types: ')
   for t,arr in pairs(self.codes) do
	  print('>>  ' .. input.event_types[t])
	  for id,name in pairs(arr) do
		 print('>>    ', id, name)
	  end
   end
end

input = {}
input.devices = {}
dofile("lua/input_device_codes.lua")

input.add = function(id, serial, name, types, codes)
   print('>> adding input device', id, serial, name)
   local d = InputDevice:new(id, serial, name, types, codes)
   input.devices[id] = d
end

input.event = function(id, type, code, value)
   local d = input.devices[id]
   if d ~= nil then
	  print('>> ' .. d.name
			   .. ' ' .. type
			   .. ' (' .. input.event_types[type]
			   .. ') ' .. code
			   .. ' (' .. input.event_codes[type][code]
			   .. ')  ' .. value)
   end
end
