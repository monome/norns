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

norns.state.resume()


-- shortcuts
run = norns.script.load
