--- Engine class
-- @module engine
-- @alias Engine
require 'norns'
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
Engine.registerCommands = function(data, count)
   local name, fmt   
   Engine.commands = {}
   for i=1,count do
      name = data[i][1]
      fmt = data[i][2]
      Engine.addCommand(i, name, fmt)
   end
end

--- add a command to the current engine
-- @param id - integer index
-- @param name - command name (string)
-- @param fmt - OSC format string (e.g. 'isf' for "int string float")
Engine.addCommand = function(id, name, fmt)
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
      func = func      
   }
end

--- load a named engine, with a callback
-- @param name - name of engine
-- @param callback - functoin to call on engine load. will receive command list
Engine.load = function(name, callback)
   -- on engine load, command report will be generated
   norns.report.commands = function(commands, count)
      Engine.registerCommands(commands, count)
      if callback then
	 callback(Engine.commands)
      end
   end
   load_engine(name)
end


--- custom getters; 
-- [] accessor returns a command function;
-- this allows e.g. engine.hz(100) 
function Engine.__index(self, idx)
   --if idx == 'name' then return rawget(Engine, name);
   if Engine.commands[idx] then
      return Engine.commands[idx].func
   else
      return rawget(Engine, idx)
   end
end

setmetatable(Engine, Engine)

--- Global Functions
-- @section globals

--- redefines engine report
norns.report.engines = function(names, count)
   Engine.register(names, count)
end

return Engine
