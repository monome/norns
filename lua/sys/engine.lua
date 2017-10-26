local Engine = {}
Engine.__index = Engine

--- constructor
-- @param props - a table of initial property values
function Engine.new(props)
      local self = setmetatable({}, Engine)
      if props.name then
	 self.props = props
	 return self
      end
   end
   print("warning: engine constructor requires at least a name")
   return nil
end

--------------------------
--- static data

-- table of available engine names
Engine.engines = {}
-- the currently loaded engine
Engine.current = nil

------------------------------
-- static methods

--- register all available engines
-- call from OSC handler
-- @param data - an array of strings
-- @param count - number of names
Engine.register = function(data, count)
   print("available engines: ")
   for i=1,count do
      print("  " .. data[i])
   end
   Engine.names = data
end

--- populate the current engine object with available commands
-- call from OSC handler
-- NB: we *can* count on the order of entries to be meaningful
-- @param data - array of [name, format]
-- @param count - number of commands
Engine.registerCommands = function(data, count)
   local e = Engine.current
   local name, fmt
   if e then
      e.props.commands = {}
      for i=1,count do
	 name = data[i][1]
	 fmt = data[i][i]
	 Engine.addCommand(i, name, fmt)
      end
   end
end

--- add a command to the current engine
-- @param id - integer index
-- @param name - command name (string)
-- @param fmt - OSC format string (e.g. 'isf' for "int string float")
Engine.addCommand = function(id, name, fmt)
   -- FIXME(?) maybe a less screwy way to do this than JIT compiling a string.
   local body
   local argstr = ""
   for i in 1,#fmt do
      argstr = argstr.."arg"..i..string.sub(fmt,i,1)..","
   end
   argstr = string.sub(argstr, 0, -2)
   local str = "Engine.current.props.commands[name] = function( "
   str = str .. argstr .. " ) "
   str = str .. " send_command( " .. id .. " , " .. argstr .. ") end "
   local func = load(str)
   if func then func()
   else print("error defining function: \n" .. str .. "\n") end
end

--- load a named engine, with a callback
-- @param name - name of engine
-- @param callback - functoin to call on engine load. will receive command list
Engine.load = function(name, callback)
   -- on engine load, command report will be generated
   norns.report.commands = function(commands, count)
      Engine.registerCommands(commands, count)
      callback(commands)
   end
   load_engine(name)
end

----------------------------------
-- instance methods

-- getters / methods
function Engine:__index(idx)
   if idx == 'name' then return self.props.name
   elseif self.props.commands[idx] then
      return self.props.commands[idx]
   else
      return rawget(self, idx)
   end
end


return Engine
