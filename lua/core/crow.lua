local util = require 'util'

norns.crow = {}

norns.crow.add = function(id, name, dev)
  print(">>>>>> norns.crow.add / " .. id .. " / " .. name)
end


norns.crow.remove = function(id)
  print(">>>>>> norns.crow.remove " .. id)
end

norns.crow.event = function(id)
  print(">> crow: " .. id)
end
