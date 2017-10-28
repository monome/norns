-----------------------
---- global variables

norns.input = {}
norns.input.devices = {}

require('input_device_codes')

-- input device class
Input = {}
Input.__index = Input

--- device add callback
-- script should redefine if it wants to handle device hotplug events
-- @param device - an Input 
Input.add = function(device)
   print("device added: ", device.id, device.name)
end

--- device remove callback
-- script should redefine if it wants to handle device hotplug events
-- @param device - an Input 
Input.remove = function(device)
   print("device removed: ", device.id, device.name)
end

-- `codes` is indexed by event type, values are subtables
-- @param id : arbitrary numberical index of the device
-- @param serial : serial device string from USB
-- @param name: device name string fromUSB
-- @param types: array of supported event types. keys are type codes, values are strings
-- @param codes: array of supported codes. each entry is a table of codes of a given type. subtables are indexed by supported code numbers; values are code names
function Input.new(id, serial, name, types, codes)
   local d = setmetatable({}, Input)
   -- print(id, serial, name, types, codes)
   d.id = id
   d.serial = serial
   d.name = name
   d.types = types
   d.codes = {}
   d.callbacks = {}
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

--- return the first available device that supports the given event
-- @param ev_type - event type (string), e.g. 'EV_KEY'
-- @param code - target event code (string), e.g. 'BTN_START'
-- @return - an Input or nil
function Input.findDeviceSupporting(ev_type, ev_code)
   local ev_type_num = norns.input.event_types_rev[ev_type]
   if ev_type_num == nil then return nil end   
   local ev_code_num = norns.input.event_codes_rev[ev_type][ev_code]
   if ev_code_num == nil then return nil end   
   -- only reason we don't call Input:supports here is to save cycles + allocations
   for i,v in pairs(norns.input.devices) do
      if v.types[ev_type_num] then
	 if v.codes[ev_type_num][ev_code_num] then return v end
      end      
   end
   return nil -- didn't find any
end

------------------------------------
--- instance methods

--- test if device supports given event type and code
-- @param ev_type - event type (string), e.g. 'EV_REL'
-- @param ev_code - event code (string), e.g. 'REL_X',
-- @return boolean
function Input:supports(ev_type, ev_code)
   local ev_type_num = norns.input.event_types_rev[ev_type]
   if ev_type_num == nil then return false end   
   local ev_code_num = norns.input.event_codes_rev[ev_type][ev_code]
   if ev_code_num == nil then return false end
   if self.types[ev_type_num] then
      if self.codes[ev_type_num][ev_code_num] then return true end
   end
   return false      
end

--- print some information about a device
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


---------------------------------------------------
--- global functions (needed for C glue)

--- add a device
-- @param id - arbitrary id number (int)
-- @param serial (string)
-- @param name (string)
-- @param types - table of event types  (int)
-- @param codes - table of table of event codes (int), indexed by type (int)
norns.input.add = function(id, serial, name, types, codes)
   local d = Input.new(id, serial, name, types, codes)
   norns.input.devices[id] = d
   if Input.add ~= nil then Input.add(d) end
end

--- remove a device
-- @param id - arbitrary id numer (int) 
norns.input.remove = function(id)
   local d = norns.input.devices[id]
   if d then
      if Input.remove then Input.remove(d) end
      norns.input.devices[id] = nil
   end
   
end

--- handle an input event
-- @param id- arbitrary id number (int)
-- @param ev_type - event type (int)
-- @param ev_code - event code (int)
-- @param value (int)
norns.input.event = function(id, ev_type, ev_code, value)
   local ev_type_name = norns.input.event_types[ev_type]
   local ev_code_name = norns.input.event_codes[ev_type][ev_code]
   print(id, ev_type_name, ev_code_name, value)
   local dev = norns.input.devices[id]
   if  dev then
      local cb = dev.callbacks[ev_code_name]
      if cb then cb(value) end
   end
end

return Input
