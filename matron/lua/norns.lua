-- startup script for norns lua environment.

-- NB: removing these module definitions will break the C->lua glue.
-- in other respects, feel free to customize.

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

print("running norns.lua")

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

-- MIDI
-- ... 
