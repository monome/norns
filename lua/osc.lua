--- osc device
-- @module osc
-- @alias OSC
require 'norns'

norns.version.osc = '0.0.0'

local tab = require 'tabutil'

local OSC = {}
OSC.__index = OSC

--- static callback when an osc event is received
-- user scripts can redefine
-- @param from : a {host, port} table with the source address
-- @param path : osc message path
-- @param args : osc message args
function OSC.event(path, args, from)
  print("incoming osc message from", from, path)
  tab.print(args)
end

--- static method to send osc event
-- @param to : a {host, port} table with the destination address
-- @param path : osc message path
-- @param args : osc message args
function OSC.send(to, path, args)
  if (args ~= nil) then
    osc_send(to, path, args)
  else
    osc_send(to, path)
  end
end

--- static method to send osc event directly to sclang
-- @param path : osc message path
-- @param args : osc message args
function OSC.send_crone(path, args)
  if (args ~= nil) then
    osc_send_crone(path, args)
  else
    osc_send_crone(path)
  end
end

--- handle an osc event
norns.osc.event = function(path, args, from)
  if OSC.event ~= nil then OSC.event(path, args, from) end
end

return OSC
