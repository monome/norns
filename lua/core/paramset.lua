--- ParamSet class
-- @module paramset

local separator = require 'core/params/separator'
local number = require 'core/params/number'
local option = require 'core/params/option'
local control = require 'core/params/control'
local file = require 'core/params/file'
local taper = require 'core/params/taper'
local trigger = require 'core/params/trigger'
local binary = require 'core/params/binary'
local group = require 'core/params/group'
local text = require 'core/params/text'

local ParamSet = {
  tSEPARATOR = 0,
  tNUMBER = 1,
  tOPTION = 2,
  tCONTROL = 3,
  tFILE = 4,
  tTAPER = 5,
  tTRIGGER = 6,
  tGROUP = 7,
  tTEXT = 8,
  tBINARY = 9,
  sets = {}
}


-- utility hack
local system_param_ids  =  {
  comp_release = true,
  monitor_level = true,
  cut_input_eng = true,
  comp_attack = true,
  COMPRESSOR = true,
  rev_cut_input = true,
  rev_return_level = true,
  tape_level = true,
  reverb = true,
  comp_post_gain = true,
  rev_pre_delay = true,
  clock_crow_out_div = true,
  compressor = true,
  cut_input_tape = true,
  rev_lf_fc = true,
  clock_crow_out = true,
  REVERB = true,
  rev_low_time = true,
  comp_threshold = true,
  rev_monitor_input = true,
  LEVELS = true,
  rev_hf_damping = true,
  rev_eng_input = true,
  input_level = true,
  rev_tape_input = true,
  clock_midi_out = true,
  comp_pre_gain = true,
  link_start_stop_sync = true,
  softcut_level = true,
  clock_reset = true,
  link_quantum = true,
  engine_level = true,
  clock_tempo = true,
  clock_source = true,
  CLOCK = true,
  output_level = true,
  clock_crow_in_div = true,
  rev_mid_time = true,
  cut_input_adc = true,
  comp_mix = true,
  SOFTCUT = true,
  monitor_mode = true,
  headphone_gain = true,
  comp_ratio = true
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
  ps.hidden = {}
  ps.lookup = {}
  ps.group = 0
  ps.action_write = nil
  ps.action_read = nil
  ps.action_delete = nil
  ParamSet.sets[ps.id] = ps
  return ps
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
      param = number.new(id, name, args.min, args.max, args.default, args.formatter, args.wrap, args.allow_pmap)
    elseif args.type == "option" then
      param = option.new(id, name, args.options, args.default, args.allow_pmap)
    elseif args.type == "control" then
      param = control.new(id, name, args.controlspec, args.formatter, args.allow_pmap)
    elseif args.type == "file" then
      param = file.new(id, name, args.path)
    elseif args.type == "taper" then
      param = taper.new(id, name, args.min, args.max, args.default, args.k, args.units)
    elseif args.type == "trigger" then
      param = trigger.new(id, name)
    elseif args.type == "binary" then
      param = binary.new(id, name, args.behavior, args.default, args.allow_pmap)
    elseif args.type == "text" then
      param = text.new(id, name, args.text)
    elseif args.type == "separator" then
      param = separator.new(id, name)
    elseif args.type == "group" then
      param = group.new(id, name, args.n)
    else
      print("paramset.add() error: unknown type")
      return nil
    end
  end

  local overwrite = true
  if self.lookup[param.id] ~= nil and param.t ~= 0 and param.t ~= 7 then
    print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
    print("!!!!! ERROR: parameter ID collision: ".. param.id)
    print("! please contact the script maintainer - this will cause a load failure in future updates")
    if system_param_ids[param.id] ~= nil then
      print("! since this is a system param ID, i am refusing to clobber it")
      return
    else
      print("! BEWARE! clobbering a script or mod param")
    end
  elseif self.lookup[param.id] ~= nil and param.t == 0 then
    if params:lookup_param(param.id).t ~= 0 then
      print("! separator ID <"..param.id.."> collides with a non-separator parameter, will not overwrite")
      overwrite = false
    elseif param.id ~= "separator" then
      print("! stealing separator ID <"..param.id.."> from earlier separator")
      overwrite = true
    end
  elseif self.lookup[param.id] ~= nil and param.t == 7 then
    if params:lookup_param(param.id).t ~= 7 then
      print("! group ID <"..param.id.."> collides with a non-group parameter, will not overwrite")
      overwrite = false
    elseif param.id ~= "group" then
      print("! stealing group ID <"..param.id.."> from earlier group")
      overwrite = true
    end
  end

  param.save = true

  table.insert(self.params, param)
  self.count = self.count + 1
  self.group = self.group - 1
  if overwrite == true then
    self.lookup[param.id] = self.count
  end
  self.hidden[self.count] = false
  if args.action then
    param.action = args.action
  end
end

--- add number.
-- @tparam string id identifier slug (no spaces)
-- @tparam string name user-facing name (can contain spaces)
-- @tparam number min minimum value
-- @tparam number max maximum value
-- @tparam number default default / initial value
-- @tparam function formatter function accepting a value and returning a string
-- @tparam boolean wrap if true, value wraps on delta; otherwise saturates
function ParamSet:add_number(id, name, min, max, default, formatter, wrap)
  self:add { param=number.new(id, name, min, max, default, formatter, wrap) }
end

--- add option.
-- @tparam string id (no spaces)
-- @tparam string name (can contain spaces)
-- @param options
-- @param default
function ParamSet:add_option(id, name, options, default)
  self:add { param=option.new(id, name, options, default) }
end

--- add control.
-- @tparam string id (no spaces)
-- @tparam string name (can contain spaces)
-- @tparam controlspec controlspec
-- @param formatter
function ParamSet:add_control(id, name, controlspec, formatter)
  self:add { param=control.new(id, name, controlspec, formatter) }
end

--- add file.
-- @tparam string id (no spaces)
-- @tparam string name (can contain spaces)
-- @tparam string path
function ParamSet:add_file(id, name, path)
  self:add { param=file.new(id, name, path) }
end

--- add text.
function ParamSet:add_text(id, name, txt)
  self:add { param=text.new(id, name, txt) }
end

--- add taper.
-- @tparam string id (no spaces)
-- @tparam string name (can contain spaces)
-- @tparam number min
-- @tparam number max
-- @param default
-- @tparam number k
-- @tparam string units
function ParamSet:add_taper(id, name, min, max, default, k, units)
  self:add { param=taper.new(id, name, min, max, default, k, units) }
end

--- add trigger.
-- @tparam string id (no spaces)
-- @tparam string name (can contain spaces)
function ParamSet:add_trigger(id, name)
  self:add { param=trigger.new(id, name) }
end

--- add binary
-- @tparam string id (no spaces)
-- @tparam string name (can contain spaces)
-- @tparam string behavior
-- @tparam number default
function ParamSet:add_binary(id, name, behavior, default)
  self:add { param=binary.new(id, name, behavior, default) }
end

--- add separator.
-- id and name are optional.
-- if neither id or name are provided,
-- separator will be named 'separator'
-- and will not have a unique parameter index.
-- separators which have their own parameter index
-- can be hidden / shown.
-- @tparam string id (no spaces)
-- @tparam string name (can contain spaces)
function ParamSet:add_separator(id, name)
  self:add { param=separator.new(id, name) }
end

--- add parameter group.
-- groups cannot be nested,
-- i.e. a group cannot be made within a group.
-- id and name are optional.
-- if neither id or name are provided,
-- group will be named 'group'
-- and will not have a unique parameter index.
-- groups which have their own parameter index
-- can be hidden / shown.
-- @tparam string id (no spaces)
-- @tparam string name (can contain spaces)
-- @tparam int n
function ParamSet:add_group(id, name, n)
  if id == nil then id = "group" end
  n = type(name) == "number" and name or (n or 1)
  if self.group < 1 then
    self:add { param=group.new(id, name, n) }
    self.group = type(name) == "number" and name or n
  else
    print("ERROR: paramset cannot nest GROUPs")
  end
end

--- print.
function ParamSet:print()
  print("paramset ["..self.name.."]")
  for k,v in pairs(self.params) do
    local name = v.name or 'unnamed' -- e.g., separators
    print(k.." "..name.." = "..v:string())
  end
end

--- list.
-- lists param id's
function ParamSet:list()
  print("paramset ["..self.name.."]")
  for k,v in pairs(self.params) do
    if v.id then
      print(v.id)
    end
  end
end

-- TODO: @scazan CHECK type here!
--- name.
-- @tparam number index
function ParamSet:get_name(index)
  return self.params[index].name or ""
end

--- id.
-- @tparam number index
function ParamSet:get_id(index)
  return self.params[index].id
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

--- set save state.
-- @param index
-- @param state set the save state for this index
function ParamSet:set_save(index, state)
  local param = self:lookup_param(index)
  param.save = state
end

--- get type.
-- @param index
function ParamSet:t(index)
  local param = self:lookup_param(index)
  if param ~= nil then
    return param.t
  end
end

--- get range
-- @param index
function ParamSet:get_range(index)
  local param = self:lookup_param(index)
  return param:get_range()
end

--- get whether or not parameter should be pmap'able
-- @param index
function ParamSet:get_allow_pmap(index)
  local param = self:lookup_param(index)
  local allow = param.allow_pmap
  if param == nil then return true end
  return allow
end

--- set visibility to hidden.
-- @param index
function ParamSet:hide(index)
  if type(index)=="string" then index = self.lookup[index] end
  self.hidden[index] = true
end

--- set visiblility to show.
-- @param index
function ParamSet:show(index)
  if type(index)=="string" then index = self.lookup[index] end
  self.hidden[index] = false
end

--- get visibility.
-- parameters are visible by default.
-- @param index
function ParamSet:visible(index)
  if type(index)=="string" then index = self.lookup[index] end
  return not self.hidden[index]
end


local function quote(s)
  return '"'..s:gsub('"', '\\"')..'"'
end

local function unquote(s)
  return s:gsub('^"', ''):gsub('"$', ''):gsub('\\"', '"')
end

-- get param object at index; useful for meta-programming tasks like changing a param once it's been created.
-- @param index
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
-- @tparam string name
function ParamSet:write(filename, name)
  filename = filename or 1
  local pset_number;
  if type(filename) == "number" then
    local n = filename
    filename = norns.state.data .. norns.state.shortname
    pset_number = string.format("%02d",n)
    filename = filename .. "-" .. pset_number .. ".pset"
  end
  print("pset >> write: "..filename)
  local fd = io.open(filename, "w+")
  if fd then
    io.output(fd)
    if name then io.write("-- "..name.."\n") end
    for _,param in ipairs(self.params) do
      if param.id and param.save and param.t ~= self.tTRIGGER and param.t ~= self.tSEPARATOR then
        io.write(string.format("%s: %s\n", quote(param.id), param:get()))
      end
    end
    io.close(fd)
    if self.action_write ~= nil then
      self.action_write(filename,name,pset_number)
    end
  else print("pset: BAD FILENAME") end
end

--- read from disk.
-- @tparam string filename either an absolute path, number (to read [scriptname]-[number].pset from local data folder) or nil (to read pset number specified by pset-last.txt in the data folder)
-- @tparam boolean silent if true, do not trigger parameter actions
function ParamSet:read(filename, silent)
  filename = filename or norns.state.pset_last
  local pset_number;
  if type(filename) == "number" then
    local n = filename
    filename = norns.state.data .. norns.state.shortname
    pset_number = string.format("%02d",n)
    filename = filename .. "-" .. pset_number .. ".pset"
  end
  print("pset >> read: " .. filename)
  local fd = io.open(filename, "r")
  if fd then
    io.close(fd)
    local param_already_set = {}
    for line in io.lines(filename) do
      if util.string_starts(line, "--") then
        params.name = string.sub(line, 4, -1)
      else
        local id, value = string.match(line, "(\".-\")%s*:%s*(.*)")

        if id and value then
          id = unquote(id)
          local index = self.lookup[id]

          if index and self.params[index] and not param_already_set[index] then
            if tonumber(value) ~= nil then
              self.params[index]:set(tonumber(value), silent)
            elseif value == "-inf" then
              self.params[index]:set(-math.huge, silent)
            elseif value == "inf" then
              self.params[index]:set(math.huge, silent)
            elseif value then
              self.params[index]:set(value, silent)
            end
            param_already_set[index] = true
          end
        end
      end
    end
    if self.action_read ~= nil then 
      self.action_read(filename,silent,pset_number)
    end
  else
    print("pset :: "..filename.." not read.")
  end
end

--- delete from disk.
-- @param filename either an absolute path, a number (for [scriptname]-[number].pset in local data folder) or nil (for default [scriptname].pset in local data folder)
-- @tparam string name
function ParamSet:delete(filename, name, pset_number)
  if type(filename) == "number" then
    local n = filename
    filename = norns.state.data .. norns.state.shortname
    filename = filename .. "-" .. string.format("%02d",n) .. ".pset"
  end
  print("pset >> delete: "..filename, name, pset_number)
  norns.system_cmd("rm "..filename)
  if self.action_delete ~= nil then
    self.action_delete(filename, name, pset_number)
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
    if v.t ~= self.tTRIGGER and not (v.t == self.tBINARY and v.behavior == 'trigger') then
      v:bang()
    end
  end
end

--- clear.
function ParamSet:clear()
  self.name = ""
  self.params = {}
  self.count = 0
  self.action_read = nil 
  self.action_write = nil
  self.action_delete = nil
  self.lookup = {}
end


return ParamSet
