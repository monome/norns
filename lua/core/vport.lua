local vport = {}

function vport.wrap_method(method)
  return function(self, ...)
    if self.device then self.device[method](self.device, ...) end
  end
end

function vport.get_unique_device_name(name, devices)
  local names = {}

  for _, device in pairs(devices) do
    names[device.name] = true
  end

  local result = name
  local suffix = 2

  while names[result] ~= nil do
    result = name.." "..suffix
    suffix = suffix + 1
  end

  return result
end

return vport
