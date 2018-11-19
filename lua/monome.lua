-- -------------------------------
-- monome device manager

local util = require 'util'
local grid = require 'grid'
local arc = require 'arc'

norns.monome = {}

norns.monome.add = function(id, serial, name, dev)
  if util.string_starts(name, "monome arc") then
    norns.arc.add(id, serial, name, dev)
  else
    norns.grid.add(id, serial, name, dev)
  end
end


norns.monome.remove = function(id)
  if arc.devices[id] then
    norns.arc.remove(id)
  else
    norns.grid.remove(id)
  end
end
