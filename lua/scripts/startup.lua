
--- add your startup code here!
print("startup.lua")
require 'norns'

--- load all the norns modules to check their versions
require 'engine'
require 'grid'
require 'input'
require 'poll'
require 'timer'
print("norns module versions: ")
  for mod,v in pairs(norns.version) do
     print (mod,v)
  end


--- test scripts for each module
-- require('test_engine')  -- OK
-- require('test_grid') -- OK
-- require('test_input') -- OK
require('test_timer') -- OK
-- require('test_amp_poll') -- OK
