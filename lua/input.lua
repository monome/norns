

InputDevice = {}
InputDevice.__index = InputDevice

function InputDevice:new(id, serial, name, types, codes)
   local d = setmetatable({}, InputDevice)
   d.id = id
   d.serial = serial
   d.name = name
   d.types = types
   d.codes = codes
   return d
end

function InputDevice:print()
   print(self.id, self.serial, self.name)
   print('>> event types: ')
   for k,v in pairs(self.types) do
	  if input.event_types[v] ~= nil then
		 print('>>  ' .. input.event_types[v] .. ' : ')
		 for l,w in pairs(self.codes[k]) do
			if input.event_types[v] == 'EV_KEY' then
			   print( '>>    ' .. input.key_btn_codes[w])
			elseif input.event_types[v] == 'EV_REL' then
			   print( '>>    ' .. input.rel_codes[w])
			elseif input.event_types[v] == 'EV_ABS' then
			   print( '>>    ' .. input.abs_codes[w])
			end
		 end
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
