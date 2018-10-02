--- ParamSet class
-- @module paramset

local separator = require 'params/separator'
local number = require 'params/number'
local option = require 'params/option'
local control = require 'params/control'
local file = require 'params/file'
local taper = require 'params/taper'
local trigger = require 'params/trigger'

local ParamSet = {
  tSEPARATOR = 0,
  tNUMBER = 1,
  tOPTION = 2,
  tCONTROL = 3,
  tFILE = 4,
  tTAPER = 5,
  tTRIGGER = 6
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
  local param = args.param
  if param == nil then
    if args.type == nil then
      print("paramset.add() error: type required")
      return nil
    elseif args.id == nil then
      print("paramset.add() error: id required")
      return nil
    end

    local id = args.id
    local name = args.name or id

    if args.number then
      param = number.new(id, name, args.min, args.max, args.default)
    elseif args.option then
      param = option.new(id, name, args.options, args.default)
    elseif args.control then
      param = control.add(id, name, args.controlspec, args.formatter)
    elseif args.file then
      param = file.add(id, name, args.path)
    elseif args.taper then
      param = taper.add(id, name, args.min, args.max, args.default, args.k, args.units)
    elseif args.trigger then
      param = trigger.add(id, name)
    else
      print("paramset.add() error: unknown type")
      return nil
    end
  end

  table.insert(self.params, param)
  self.count = self.count + 1
  self.lookup[param.id] = self.count
  if args.action then
    param.action = args.action
  end
end

--- add number
function ParamSet:add_number(id, name, min, max, default)
  self:add { param=number.new(id, name, min, max, default) }
end

--- add option
function ParamSet:add_option(id, name, options, default)
  p = option.new(id, name, options, default)
  self:add { param=option.new(id, name, options, default) }
end

--- add control
function ParamSet:add_control(id, name, controlspec, formatter)
  self:add { param=control.new(id, name, controlspec, formatter) }
end

--- add file
function ParamSet:add_file(id, name, path)
  self:add { param=file.new(id, name, path) }
end

--- add taper
function ParamSet:add_taper(id, name, min, max, default, k, units)
  self:add { param=taper.new(id, name, min, max, default, k, units) }
end

--- add trigger
function ParamSet:add_trigger(id, name)
  self:add { param=trigger.new(id, name) }
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
    if param.id and param.t ~= self.tTRIGGER then
      io.write(string.format("%s: %s\n", quote(param.id), param:get()))
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
      local id, value = string.match(line, "(\".-\")%s*:%s*(.*)")

      if id and value then
        id = unquote(id)
        local index = self.lookup[id]

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
