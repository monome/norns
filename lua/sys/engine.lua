local Engine = {}
Engine.__index = Engine

-- constructor
function Engine.new(props)
      local self = setmetatable({}, Poll)
      if props.id && props.name then
	 self.props = props
	 return self
      end
   end
   print("warning: Poll constructor requires at least name and id properties")
   return nil
end

--------------------------
--- static data

-- table of available engine names
Engine.names = {}
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
      Engine.names = data[i]	    
   end
end

--- populate an engine instance with available commands
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
   -- FIXME(?) maybe a less screwy way to do this than JIT co
   mpiling a string.
   local body
   local argstr = ""
   for i in 1,#fmt do
      argstr = argstr.."arg"..i..string.sub(fmt,i,1)..","
   end
   argstr = string.sub(argstr, 0, -2)
   local str = "Engine.current.commands[name] = function( "
   str = str .. argstr .. " ) "
   str = str .. " send_command( " .. id .. " , " .. argstr .. ") end "
   local func = load(str)
   if func then func()
   else print("error defining function: \n" .. str .. "\n") end
end


----------------------------------
-- instance methods

-- getters

-- setters

return Engine
