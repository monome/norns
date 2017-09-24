--[[
   helpers.lua
   utilities and helper functions
--]]

-- define a function with arbitrary name and parameter list
-- target: any valid lvalue expression to hold the function
-- args: table of strings
-- body: function body
function defineFunction(target, args, body)
   local str = target .. " = function ("
   for k,v in pairs(args) do
	  str = str .. v .. ","
   end
   str = string.sub(str, 0, -2) -- strip trailing ','
   str = str .. ") " .. body .. " end \n"

   local func = load(str)
   if(func ~= nil) then func()
   else print("error defining function: \n" .. str .. "\n") end
end


-- given an engine command string and format string,
-- define a function in the engine command table
function defineEngineCommand(idx, name, fmt)
   local args = {};
   local target = "norns.engine." .. name
   local body = 'send_command(' .. idx .. ','

   for i=1,#fmt do
	  args[i] = "arg"..i
	  body = body .. args[i] .. ','
   end
   body = string.sub(body, 0, -2) -- zap trailing ','
   body = body .. ')'
   defineFunction(target, args, body)
end

-- add engine commands. this is our default handler for command reports
function addEngineCommands(commands, count)
   norns.engine = {} -- clear existing commands
   norns.engine.commands = commands
   for i=1,count do
      defineEngineCommand(i, commands[i][1], commands[i][2])
   end
end
