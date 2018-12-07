--- File class
-- @module file

local File = {}
File.__index = File

local tFILE = 4

function File.new(id, name, path)
  local o = setmetatable({}, File)
  o.t = tFILE
  o.id = id
  o.name = name
  o.path = path or '-'
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
  if self.path == '-' then return "-" end
  local display = self.path:match("[^/]*$") -- strip path from name
  return display
end


return File
