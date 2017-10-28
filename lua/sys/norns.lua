--- norns.lua;
-- main norns script.
-- defines top-level global tables and functions needed by other modules

norns = {}
norns.version = {}
norns.version.norns = "0.0.1"

print("running norns.lua")

--- startup function will be run after I/O subsystems are initialized, 
-- but before I/O event loop starts ticking
startup = function()
   require('startup')
end-----------------------------
--- system state tables


--- Global Tables
-- @section global_tables

--- table of descriptor report callbacks
norns.report = {}

---------------------------------------------
--- report callbacks;
-- these functions called from C with descriptor data.
-- individual modules redefine them as appropriate.
norns.report.engines = function(names, count) end
norns.report.commands = function(commands, count) end
norns.report.polls = function(polls, count) end

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
