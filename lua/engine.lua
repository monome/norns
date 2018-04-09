--- Engine class
-- @module engine
-- @alias Engine
local tab = require 'tabutil'

norns.version.engine = '0.0.2'

local Engine = {}
-- ------------------------
-- static data

-- registered engine names
Engine.names = {}
-- currently loaded name
Engine.name = nil
-- current command table
Engine.commands = {}
-- current parameters table
Engine.parameters = {}

-- ----------------------------
-- static methods

--- register all available engines;
-- called from OSC handler
-- @param data - an array of strings
-- @param count - number of names
Engine.register = function(data, count)
  print("available engines ("..count.."): ")
  for i=1,count do
    print("  " .. data[i])
  end
  Engine.names = data
end

--- populate the current engine object with available commands;
-- called from OSC handler
-- NB: we *can* count on the order of entries to be meaningful
-- @param data - array of [name, format]
-- @param count - number of commands
Engine.register_commands = function(data, count)
  local name, fmt
  print('Engine.register_commands; count: '..count)
-- Engine.numCommands = count;
  Engine.commands = {}
  for i=1,count do
    name = data[i][1]
    fmt = data[i][2]
    Engine.add_command(i, name, fmt)
  end
end

--- add a command to the current engine
-- @param id - integer index
-- @param name - command name (string)
-- @param fmt - OSC format string (e.g. 'isf' for "int string float")
Engine.add_command = function(id, name, fmt)
  local func = function(...)
    local arg={...}
    if select("#",...) ~= #fmt then
   print("warning: wrong count of arguments for command '"..name.."'")
    end
    send_command(id, table.unpack(arg))
  end
  Engine.commands[name] = {
    id = id,
    name = name,
    fmt = fmt,
    func = func,
  }
end

Engine.list_commands = function()
  print("--- engine commands ---")
  local sorted = tab.sort(Engine.commands)
  for i,n in ipairs(sorted) do
    print(Engine.commands[n].name ..'  ('.. Engine.commands[n].fmt .. ')')
  end
  print("------\n")
end

--- populate the current engine object with available parameters;
-- called from OSC handler
-- NB: we *can* count on the order of entries to be meaningful
-- @param data - array of [name, bus, minval, maxval, warp, step, default, units]
-- @param count - number of parameters
Engine.register_parameters = function(data, count)
  print('Engine.register_parameters; count: '..count)
  Engine.parameters = {}
  for i=1,count do
    local name = data[i][1]
    local bus = data[i][2]
    local minval = data[i][3]
    local maxval = data[i][4]
    local warp = data[i][5]
    local step = data[i][6]
    local default = data[i][7]
    local units = data[i][8]
    Engine.add_parameter(i, name, bus, minval, maxval, warp, step, default, units)
  end
end

--- add a parameter to the current engine
-- @param id - integer index
-- @param name - parameter name (string)
-- @param bus - crone controlbus index (number)
-- @param minval - the minimum value of the range (number)
-- @param maxval - the maximum value of the range (number)
-- @param warp - a string describing the warp (exponential, linear) (string)
-- @param step - the smallest possible increment (number)
-- @param default - the default value (number)
-- @param units - the units, e.g. "Hz" possible for use as a ui label (string)
Engine.add_parameter = function(id, name, bus, minval, maxval, warp, step, default, units)
  local controlspec = ControlSpec.new(minval, maxval, warp, step, default, units)
  local func = function(value)
    set_parameter_value(id, controlspec.constrain(value)) -- TODO: use bus instead of id? is constrain implemented in lua ControlSpec?
  end
  Engine.parameters[name] = {
    id = id,
    name = name,
    bus = bus,
    controlspec = controlspec,
    func = func,
  }
end

Engine.list_parameters = function()
  print("--- engine parameters ---")
  local sorted = tab.sort(Engine.parameters)
  for i,n in ipairs(sorted) do
    local param = Engine.parameters[n]
    local spec = param.spec
    print(param.name ..'  ('.. param.bus .. ', '.. spec.minval ..', '.. spec.maxval ..', '.. spec.warp ..', '.. spec.step ..', '.. spec.default ..', '.. spec.units ..')')
  end
  print("------\n")
end

--- load a named engine, with a callback
-- @param name - name of engine
-- @param callback - function to call on engine load. will receive command list
Engine.load = function(name, callback)
  if type(callback) == 'function' then
    norns.report.did_engine_load = function()
      print("Engine: norns.report.did_engine_load callback")
      callback()
    end
  end
  load_engine(name)
end

--- custom getters;
-- [] accessor returns a command function;
-- this allows e.g. engine.hz(100)
function Engine.__index(self, idx)
  if Engine.commands[idx] then
    return Engine.commands[idx].func
  else
    return rawget(Engine, idx)
  end
end

setmetatable(Engine, Engine)

return Engine
