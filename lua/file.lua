--- File class
-- @module number

local File = {}
File.__index = File

local tFILE = 1

function File.new(name, path)
  local o = setmetatable({}, File)
  o.t = tFILE
  o.name = name
  o.path = path or ''
  o.action = function() end
  return o
end 

function File:get()
  return self.path
end

function File:set(v)
  if self.path ~= v then
    self.path = v
    self:bang()
  end
end

function File:delta(d)
  --noop
end

function File:set_default()
  --noop
end

function File:bang()
  self.action(self.path)
end

function File:string()
  return self.path
end


return File
