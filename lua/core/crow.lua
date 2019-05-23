local util = require 'util'

norns.crow = {}

norns.crow.add = function(id, name, dev)
  print(">>>>>> norns.crow.add / " .. id .. " / " .. name)
  norns.crow.dev = dev
end

norns.crow.remove = function(id)
  print(">>>>>> norns.crow.remove " .. id)
end

norns.crow.event = function(id, line)
  print("crow (" .. id .. ") : " .. string.sub(line,1,-2)) -- substring to ditch newline
end


norns.crow.send = function(cmd)
  _norns.crow_send(norns.crow.dev,cmd)  
end
