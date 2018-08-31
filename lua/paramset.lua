--- ParamSet class
-- @module paramset

local separator = require 'params/separator'
local number = require 'params/number'
local option = require 'params/option'
local control = require 'params/control'
local file = require 'params/file'
local taper = require 'params/taper'

local ParamSet = {
  tSEPARATOR = 0,
  tNUMBER = 1,
  tOPTION = 2,
  tCONTROL = 3,
  tFILE = 4,
  tTAPER = 5,
}

ParamSet.__index = ParamSet

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

--- add separator
function ParamSet:add_separator()
  table.insert(self.params, separator.new())
  self.count = self.count + 1
end

--- add generic parameter
function ParamSet:add(args)
  local param = args.param -- param is mandatory
  table.insert(self.params, param)
  self.count = self.count + 1
  self.lookup[param.name] = self.count
  if args.action then -- action is optional
    param.action = args.action
  end
end

--- add number
function ParamSet:add_number(name, min, max, default)
  self:add { param=number.new(name, min, max, default) }
end

--- add option
function ParamSet:add_option(name, options, default)
  self:add { param=option.new(name, options, default) }
end

--- add control
function ParamSet:add_control(name, controlspec, formatter)
  self:add { param=control.new(name, controlspec, formatter) }
end

--- add file
function ParamSet:add_file(name, path)
  self:add { param=file.new(name, path) }
end

--- add taper
function ParamSet:add_taper(name, min, max, default, k, units)
  self:add { param=taper.new(name, min, max, default, k, units) }
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

--- set_raw (for control types only)
function ParamSet:set_raw(index, v)
  if type(index) == "string" then index = self.lookup[index] end
  self.params[index]:set_raw(v)
end

--- get
function ParamSet:get(index)
  if type(index) == "string" then index = self.lookup[index] end
  return self.params[index]:get()
end

--- get_raw (for control types only)
function ParamSet:get_raw(index)
  if type(index) == "string" then index = self.lookup[index] end
  return self.params[index]:get_raw()
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

--- get type
function ParamSet:t(index)
  return self.params[index].t
end

local function quote(s)
  return '"'..s:gsub('"', '\\"')..'"'
end

local function unquote(s)
  return s:gsub('^"', ''):gsub('"$', ''):gsub('\\"', '"')
end

--- write to disk
-- @param filename relative to data_dir
function ParamSet:write(filename)
  -- check for subfolder in filename, create subfolder if it doesn't exist
  local subfolder, found = string.gsub(filename,"/(.*)","")
  if found==1 then
    local fd = io.open(data_dir..subfolder,"r")
    if fd then
      io.close(fd)
    else
      print("creating subfolder")
      os.execute("mkdir "..data_dir..subfolder)
    end
  end
  -- write file
  local fd = io.open(data_dir..filename, "w+")
  io.output(fd)
  for k,param in pairs(self.params) do
    if param.name then
      io.write(string.format("%s: %s\n", quote(param.name), param:get()))
    end
  end
  io.close(fd)
end

--- read from disk
-- @param filename relative to data_dir
function ParamSet:read(filename)
  local fd = io.open(data_dir..filename, "r")
  if fd then
    io.close(fd)
    for line in io.lines(data_dir..filename) do
      local name, value = string.match(line, "(\".-\")%s*:%s*(.*)")

      if name and value then
        name = unquote(name)
        local index = self.lookup[name]

        if index then
          if tonumber(value) ~= nil then
            self.params[index]:set(tonumber(value))
          elseif value == "-inf" then
            self.params[index]:set(-math.huge)
          elseif value == "inf" then
            self.params[index]:set(math.huge)
          elseif value then
            self.params[index]:set(value)
          end
        end
      end

    end
  else
    print("paramset: "..filename.." not read.")
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
