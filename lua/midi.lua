--- midi devices
-- @module midi
-- @alias Midi
require 'norns'
norns.version.midi = '0.0.0'

--- add a device
norns.midi.add = function()
end

--- remove a device
norns.midi.remove = function()
end

--- handle a midi event
norns.midi.event = function(status, data1, data2)
  print("incoming midi message",
    string.format("%X", status),
    string.format("%X", data1),
    string.format("%X", data2))
end
