--- osc device
-- @module osc
-- @alias OSC
require 'norns'

norns.version.osc = '0.0.0'

local tab = require 'tabutil'

--- handle an osc event
norns.osc.event = function(addr, path, args)
  print("incoming osc message from", addr, path)
  tab.print(args)
end
