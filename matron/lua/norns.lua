-- startup script for norns lua environment.

-- NB: removing these function/module definitions will break the C->lua glue.
-- in other respects, feel free to customize.

print("running norns.lua")

-- this function will be run next
-- for example
startup = function()
   print("running user startup code")
   -- define your own startup routine here
   -- ( likely just: dofile("mycustomscript.lua") )
   -- it will be run after I/O subsystems are initialized,
   -- but before I/O event loop starts ticking
   dofile('lua/sticksine.lua')
   --dofile('lua/test.lua')
end

--------------------------
-- define default event handlers.
-- user scripts should redefine. 

-- encoder events
encoder = {}
--...

-- button events
button = {}
--...

-- grid events
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

-- HID events
joystick = {}

joystick.axis = function(stick, ax, val)
   print("stick " .. stick .. "; axis " .. ax .. "; value " .. val)
end

joystick.button = function(stick, but, state)
   print("stick " .. stick .. "; button " .. but .. "; state " .. state)
end

-- mouse/KB
--... 

-- MIDI
-- ... 

-- timer handlers
timer = {}
for i=1,16 do
   timer[i] = function(count)
	  print("handling timer id " .. i .. ", count " .. count)
   end
end

-- handlers for descriptor reports
report = {}

report.buffer = function(names, count)
   print(count .. " buffers: ")
   for i=1,count do
	  print(i .. ": "..names[i])
   end
end

report.engine = function(names, count)
   print(count .. " engines: ")
   for i=1,count do
	  print(i .. ": "..names[i])
   end
end


report.param = function(names, count)
   print(count .. " params: ")
   for i=1,count do
	  print(i .. ": "..names[i])
   end
end
