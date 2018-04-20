--- ParamSet class
-- @module paramset

local number = require 'number'
local option = require 'option'
local control = require 'control'
local file = require 'file' 

local ParamSet = {}
ParamSet.__index = ParamSet

local tNUMBER = 1
local tOPTION = 2
local tCONTROL = 3
local tFILE = 3

--- constructor
-- @param name
function ParamSet.new(name)
  local ps = setmetatable({}, ParamSet)
  ps.name = name or ""
  ps.params = {}
  ps.count = 0
  ps.lookup = {}
  return ps
end

--- add number
function ParamSet:add_number(name, min, max, default)
  table.insert(self.params, number.new(name, min, max, default))
  self.count = self.count + 1
  self.lookup[name] = self.count
end

--- add option
function ParamSet:add_option(name, options, default)
  table.insert(self.params, option.new(name, options, default))
  self.count = self.count + 1
  self.lookup[name] = self.count
end

--- add control
function ParamSet:add_control(name, controlspec, formatter)
  table.insert(self.params, control.new(name, controlspec, formatter))
  self.count = self.count + 1
  self.lookup[name] = self.count
end

--- add file
function ParamSet:add_file(name, path)
  table.insert(self.params, file.new(name, path))
  self.count = self.count + 1
  self.lookup[name] = self.count
end

--- print
function ParamSet:print()
  print("paramset ["..self.name.."]")
  for k,v in pairs(self.params) do
    print(k.." "..v.name.." = "..v:string())
  end
end

--- name
function ParamSet:get_name(index)
  return self.params[index].name
end

--- string
function ParamSet:string(index)
  if type(index) == "string" then index = self.lookup[index] end
  return self.params[index]:string()
end

--- set
function ParamSet:set(index, v)
  if type(index) == "string" then index = self.lookup[index] end
  self.params[index]:set(v)
end

--- get
function ParamSet:get(index)
  if type(index) == "string" then index = self.lookup[index] end
  return self.params[index]:get()
end

--- delta
function ParamSet:delta(index, d)
  if type(index) == "string" then index = self.lookup[index] end
  self.params[index]:delta(d)
end

--- set action
function ParamSet:set_action(index, func)
  if type(index) == "string" then index = self.lookup[index] end
  self.params[index].action = func
end


 
--- write to disk
-- @param filename relative to data_dir
function ParamSet:write(filename) 
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
function ParamSet:read(filename)
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
function ParamSet:bang()
  for k,v in pairs(self.params) do
    v:bang()
  end
end 

--- clear
function ParamSet:clear()
  self.name = ""
  self.params = {}
  self.count = 0
end

return ParamSet
