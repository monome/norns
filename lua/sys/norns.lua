--- norns.lua;
-- main norns script.
-- defines top-level global tables and functions needed by other modules

norns = {}
norns.version = {}
norns.version.norns = "0.0.0"

print("norns.lua")

--- startup function will be run after I/O subsystems are initialized, 
-- but before I/O event loop starts ticking
startup = function()
   require('startup')
end
--- Global Functions
-- @section global_functions

--- global functions required by the C interface; 
-- we "declare" these here with placeholders;
-- indeividiual modules will redefine them as needed.


--- monome device callbacks
norns.monome = {}
--- monome device added
norns.monome.add = function(id, serial, name, dev)
   print("norns.monome.add "..id, serial, name, dev)
end
--- monome device removed
norns.monome.remove = function(id)
   print("norns.monome.remove "..id)
end

--- grid device callbacks
norns.grid = {}
--- grid key event
norns.grid.key = function(id, x, y, val)
   print("norns.grid.key ", id,x,y,val)
end

--- report callbacks
norns.report = {}
norns.report.engines = function(names, count)
   print("norns.report.engines ", names, count)   
end
norns.report.commands = function(commands, count)
   print("norns.report.commands ", commands, count)
end
norns.report.polls = function(polls, count) 
   print("norns.report.polls ", commands, count)
end

--- print all module versions
norns.version_print = function()
  for key,value in pairs(norns.version) do
    print(key .. ": "  .. value)
  end
end

--- load a user script
-- @param name (string) - name of the script (without extension)
norns.load_script = function(script)
   if norns.cleanup then norns.cleanup() end
   dofile(script_dir..script..".lua")
end

return norns
