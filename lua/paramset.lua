--- Paramset class
-- @module paramset

local Paramset = {}
Paramset.__index = Paramset

local tNUMBER = 1
local tOPTION = 2
local tPARAM = 3

--- constructor
-- @param name
function Paramset.new(name)
  local ps = setmetatable({}, Paramset)
  ps.name = name or ""
  ps.params = {}
  ps.count = 0
  ps.lookup = {}
  return ps
end

--- add number
function Paramset:add_number(name, min, max, default)
  table.insert(self.params, number.new(name, min, max, default))
  self.count = self.count + 1
  self.lookup[name] = self.count
end

--- add option
function Paramset:add_option(name, options, default)
  table.insert(self.params, option.new(name, options, default))
  self.count = self.count + 1
  self.lookup[name] = self.count
end

--- add param
function Paramset:add_param(name, controlspec, formatter)
  table.insert(self.params, param.new(name, controlspec, formatter))
  self.count = self.count + 1
  self.lookup[name] = self.count
end

--- print
function Paramset:print()
  print("paramset ["..self.name.."]")
  for k,v in pairs(self.params) do
    print(k.." "..v.name.." = "..v:string())
  end
end

--- name
function Paramset:get_name(index)
  return self.params[index].name
end

--- string
function Paramset:string(index)
  if type(index) == "string" then index = self.lookup[index] end
  return self.params[index]:string()
end

--- set
function Paramset:set(index, v)
  if type(index) == "string" then index = self.lookup[index] end
  self.params[index]:set(v)
end

--- get
function Paramset:get(index)
  if type(index) == "string" then index = self.lookup[index] end
  return self.params[index]:get()
end

--- delta
function Paramset:delta(index, d)
  if type(index) == "string" then index = self.lookup[index] end
  self.params[index]:delta(d)
end

--- set action
function Paramset:set_action(index, func)
  if type(index) == "string" then index = self.lookup[index] end
  self.params[index].action = func
end


 
--- write to disk
-- @param filename relative to data_dir
function Paramset:write(filename) 
  local fd=io.open(data_dir .. filename,"w+")
  io.output(fd)
  for k,v in pairs(self.params) do
    io.write(k..","..v:get().."\n")
    --print(k..","..v:get())
  end
  io.close(fd)
end

--- read from disk
-- @param filename relative to data_dir
function Paramset:read(filename)
  local fd=io.open(data_dir .. filename,"r")
  if fd then
    io.close(fd)
    for line in io.lines(data_dir .. filename) do
      --print(line)
      k,v = line:match("([^,]+),([^,]+)")
      self.params[tonumber(k)]:set(tonumber(v))
    end 
  end 
end

--- bang all params
function Paramset:bang()
  for k,v in pairs(self.params) do
    v:bang()
  end
end 

--- clear
function Paramset:clear()
  self.name = ""
  self.params = {}
  self.count = 0
end

return Paramset
