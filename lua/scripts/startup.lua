--- add your startup code here!
print("startup.lua")
require 'norns'

--- load all the norns modules to check their versions
require 'engine'
require 'grid'
require 'input'
require 'hid'
require 'poll'
require 'metro'
print("norns module versions: ")
  for mod,v in pairs(norns.version) do
     print (mod,v)
  end

norns.state.resume()


-- shortcuts
run = norns.script.load
stop = norns.script.cleanup
