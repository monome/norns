--- ParamSet class
-- @classmod paramset

local separator = require 'core/params/separator'
local number = require 'core/params/number'
local option = require 'core/params/option'
local control = require 'core/params/control'
local file = require 'core/params/file'
local taper = require 'core/params/taper'
local trigger = require 'core/params/trigger'

local ParamSet = {
  tSEPARATOR = 0,
  tNUMBER = 1,
  tOPTION = 2,
  tCONTROL = 3,
  tFILE = 4,
  tTAPER = 5,
  tTRIGGER = 6,
  sets = {}
}

ParamSet.__index = ParamSet

--- constructor.
-- @tparam string id
-- @tparam string name
function ParamSet.new(id, name)
  local ps = setmetatable({}, ParamSet)
  ps.id = id or ""
  ps.name = name or ""
  ps.params = {}
  ps.count = 0
  ps.lookup = {}
  ParamSet.sets[ps.id] = ps
  return ps
end

--- add separator.
function ParamSet:add_separator()
  table.insert(self.params, separator.new())
  self.count = self.count + 1
end

--- add generic parameter.
-- helper function to add param to paramset
-- two uses:
-- - pass "param" table with optional "action" function
-- - pass keyed table to generate "param" table. required keys are "type" and "id"
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

    if args.type == "number"  then
      param = number.new(id, name, args.min, args.max, args.default, args.formatter)
    elseif args.type == "option" then
      param = option.new(id, name, args.options, args.default)
    elseif args.type == "control" then
      param = control.new(id, name, args.controlspec, args.formatter)
    elseif args.type == "file" then
      param = file.new(id, name, args.path)
    elseif args.type == "taper" then
      param = taper.new(id, name, args.min, args.max, args.default, args.k, args.units)
    elseif args.type == "trigger" then
      param = trigger.new(id, name)
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

--- add number.
-- @tparam string id
-- @tparam string name
-- @tparam number min
-- @tparam number max
-- @param default
-- @param formatter
function ParamSet:add_number(id, name, min, max, default, formatter)
  self:add { param=number.new(id, name, min, max, default, formatter) }
end

--- add option.
-- @tparam string id
-- @tparam string name
-- @param options
-- @param default
function ParamSet:add_option(id, name, options, default)
  self:add { param=option.new(id, name, options, default) }
end

--- add control.
-- @tparam string id
-- @tparam string name
-- @tparam controlspec controlspec
-- @param formatter
function ParamSet:add_control(id, name, controlspec, formatter)
  self:add { param=control.new(id, name, controlspec, formatter) }
end

--- add file.
-- @tparam string id
-- @tparam string name
-- @tparam string path
function ParamSet:add_file(id, name, path)
  self:add { param=file.new(id, name, path) }
end

--- add taper.
-- @tparam string id
-- @tparam string name
-- @tparam number min
-- @tparam number max
-- @param default
-- @tparam number k
-- @tparam string units
function ParamSet:add_taper(id, name, min, max, default, k, units)
  self:add { param=taper.new(id, name, min, max, default, k, units) }
end

--- add trigger.
-- @tparam string id
-- @tparam string name
function ParamSet:add_trigger(id, name)
  self:add { param=trigger.new(id, name) }
end

--- print.
function ParamSet:print()
  print("paramset ["..self.name.."]")
  for k,v in pairs(self.params) do
    local name = v.name or 'unnamed' -- e.g., separators
    print(k.." "..name.." = "..v:string())
  end
end

-- TODO: @scazan CHECK type here!
--- name.
-- @tparam number index
function ParamSet:get_name(index)
  return self.params[index].name
end

--- string.
-- @param index
function ParamSet:string(index)
  local param = self:lookup_param(index)
  return param:string()
end

--- set.
-- @param index
-- @param v value
-- @tparam boolean silent
function ParamSet:set(index, v, silent)
  local param = self:lookup_param(index)
  return param:set(v, silent)
end

--- set_raw (for control types only).
-- @param index
-- @param v value
-- @tparam boolean silent
function ParamSet:set_raw(index, v, silent)
  local param = self:lookup_param(index)
  param:set_raw(v, silent)
end

--- get.
-- @param index
function ParamSet:get(index)
  local param = self:lookup_param(index)
  return param:get()
end

--- get_raw (for control types only).
-- @param index
function ParamSet:get_raw(index)
  local param = self:lookup_param(index)
  return param:get_raw()
end

--- delta.
-- @param index
-- @tparam number d delta
function ParamSet:delta(index, d)
  local param = self:lookup_param(index)
  param:delta(d)
end

--- set action.
-- @param index
-- @tparam function func set the action for this index
function ParamSet:set_action(index, func)
  local param = self:lookup_param(index)
  param.action = func
end

--- get type.
-- @param index
function ParamSet:t(index)
  return self.params[index].t
end

local function quote(s)
  return '"'..s:gsub('"', '\\"')..'"'
end

local function unquote(s)
  return s:gsub('^"', ''):gsub('"$', ''):gsub('\\"', '"')
end

function ParamSet:lookup_param(index)
  if type(index) == "string" and self.lookup[index] then
    return self.params[self.lookup[index]]
  elseif self.params[index] then
    return self.params[index]
  else
    error("invalid paramset index: "..index)
  end
end

--- write to disk.
-- @param filename either an absolute path, a number (to write [scriptname]-[number].pset to local data folder) or nil (to write default [scriptname].pset to local data folder)
function ParamSet:write(filename)
  filename = filename or 0
  if type(filename) == "number" then
    local n = filename
    filename = norns.state.data .. norns.state.shortname
    if n > 0 then
      filename = filename .. "-" .. string.format("%02d",n)
    end
    filename = filename .. ".pset"
  end
  print("pset >> write: "..filename)
  local fd = io.open(filename, "w+")
  io.output(fd)
  for _,param in pairs(self.params) do
    if param.id and param.t ~= self.tTRIGGER then
      io.write(string.format("%s: %s\n", quote(param.id), param:get()))
    end
  end
  io.close(fd)
end

--- read from disk.
-- @param filename either an absolute path, number (to read [scriptname]-[number].pset from local data folder) or nil (to read default [scriptname].pset from local data folder)
function ParamSet:read(filename)
  filename = filename or 0
  if type(filename) == "number" then
    local n = filename
    filename = norns.state.data .. norns.state.shortname
    if n > 0 then
      filename = filename .. "-" .. string.format("%02d",n)
    end
    filename = filename .. ".pset"
  end
  print("pset >> read: " .. filename)
  local fd = io.open(filename, "r")
  if fd then
    io.close(fd)
    for line in io.lines(filename) do
      local id, value = string.match(line, "(\".-\")%s*:%s*(.*)")

      if id and value then
        id = unquote(id)
        local index = self.lookup[id]

        if index and self.params[index] then
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
    print("pset :: "..filename.." not read.")
  end
end

--- read default pset if present.
function ParamSet:default()
  self:read()
  self:bang()
end

--- bang all params.
function ParamSet:bang()
  for _,v in pairs(self.params) do
    if v.t ~= self.tTRIGGER then
      v:bang()
    end
  end
end

--- clear.
function ParamSet:clear()
  self.name = ""
  self.params = {}
  self.count = 0
end

return ParamSet
