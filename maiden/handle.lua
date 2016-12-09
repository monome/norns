-- table of handler functions (record style)
handle = {}
handle.press = function(x, y)
   print ("press " .. x .. " " .. y)
end

handle.lift = function(x, y)
   print ("lift " .. x .. " " .. y)
end

handle.connect = function()
   print ("connect " .. x .. y)
end

handle.disconnect = function()
   print ("disconnect " .. x .. y)
end
