norns.version.test_input_devices = "0.0.1"

-- print everything about every input device
input.add = function (device)
   print("added input device: ")
   device:print()
end

-- print everything about every input event
input.event = function(id, type, code, value)
   local d = norns.input.devices[id]
   if d ~= nil then
	  --[[
	  print('>> ' .. d.name
			   .. ' ' .. type
			   .. ' (' .. norns.input.event_types[type]
			   .. ') ' .. code
			   .. ' (' .. norns.input.event_codes[type][code]
			   .. ')  ' .. value)
	  --]]
   end
end
