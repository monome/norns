-- File class
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
  o:loaddir()
  return o
end

function File:get()
  return self.path
end

function File:set(v, silent)
  local silent = silent or false
  if self.path ~= v then
    self.path = v
    if self.path ~= '-' then
      self:loaddir()
    else
      self.dir = nil
      self.dirfiles = nil
      self.dirpos = nil
    end
    if silent==false then self:bang() end
  end
end

function File:delta(d)
  if self.dirpos ~= nil then
    if d > 0 then
      if self.dirpos < #self.dirfiles then
        self.dirpos = self.dirpos + 1
      end
    elseif d < 0 then
      if self.dirpos > 1 then
        self.dirpos = self.dirpos - 1
      end
    end
    local newfile = self.dirfiles[self.dirpos]
    self:set(self.dir .. self.dirfiles[self.dirpos])
  end
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

function File:loaddir()
  self.dir = self.path:match("(.*/)")
  if self.dir == nil then
    return
  end

  local name = self.path:match("[^/]*$")
  local files = util.scandir(self.dir)
  self.dirfiles = {}
  for _, fname in ipairs(files) do
    if not string.find(fname, '/') then
      self.dirfiles[#self.dirfiles + 1] = fname
      if fname == name then
        self.dirpos = #self.dirfiles
      end
    end
  end
end


return File
