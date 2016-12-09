-- table of handler functions (record style)
handle = {}
handle.press = function(x, y)
   print ("press " .. x .. " " .. y)
   monome_set_led(x, y, 1);
end

handle.lift = function(x, y)
   print ("lift " .. x .. " " .. y)
      monome_set_led(x, y, 1);
end

handle.connect = function()
   print ("connect " .. x .. y)
end

handle.disconnect = function()
   print ("disconnect " .. x .. y)
end
