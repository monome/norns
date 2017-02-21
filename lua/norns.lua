--[[ 
   norns.lua 
   startup script for norns lua environment.

   NB: removal of any function/module definitions will break the C->lua glue.
   in other respects, feel free to customize.
--]]


-- utilities and helpers
dofile('lua/helpers.lua')

print("running norns.lua")

-- this function will be run after I/O subsystems are initialized,
-- but before I/O event loop starts ticking
startup = function()
   print("running user startup code")
   -- define your own startup routine here
   -- ( likely just: dofile("mycustomscript.lua") )
   dofile('lua/sticksine.lua')
   
end

--------------------------
-- define default event handlers.
-- user scripts should redefine. 

-- tbale of encoder event handlers
encoder = {}
--...

-- table of button event handlers
button = {}
--...

-- table of grid event handlers
grid = {}
grid.press = function(x, y)
   print ("press " .. x .. " " .. y)
   grid_set_led(x, y, 1);
end

grid.lift = function(x, y)
   print ("lift " .. x .. " " .. y)
   grid_set_led(x, y, 0);
end

grid.connect = function()
   print ("grid connect ")
end

grid.disconnect = function()
   print ("grid disconnect ")
end

-- arc
--...

-- table of joystick event handlers
joystick = {}

print('asasigning default joystick handlers')
joystick.axis = function(stick, ax, val)
   print("stick " .. stick .. "; axis " .. ax .. "; value " .. val)
end

joystick.button = function(stick, but, state)
   print("stick " .. stick .. "; button " .. but .. "; state " .. state)
end

joystick.hat = function(stick, hat, val)
   print("stick " .. stick .. "; hat " .. hat .. "; value " .. val)
end


joystick.ball = function(stick, ball, xrel, yrel)
   print("stick " .. stick .. "; ball " .. ball .. "; xrel " .. xrel .. "; yrel " .. yrel)
end

-- mouse/KB
--... 

-- MIDI
-- ... 

-- table 

-- table of handlers for descriptor reports
report = {}

report.engines = function(names, count)
   print(count .. " engines: ")
   for i=1,count do
	  print(i .. ": "..names[i]) 
   end
end

report.commands = function(commands, count)
   for i=1,count do
	  print(i .. ": " .. commands[i][1] .. " (" .. commands[i][2] .. ")")
	  defineEngineCommand(i, commands[i][1], commands[i][2])
   end
   
end


-- table of engine commands
engine = {}
-- shortcut
e = engine

-- timer handler
-- FIXME? : could be a table if that is preferable.
timer = function(idx, count)
   print("timer " .. idx .. " : " .. count)
end
