--- add your startup code here!

--- load all the norns modules to check their versions
require 'engine'
require 'grid'
require 'hid'
require 'poll'
require 'metro'
require 'file'
require 'map'
require 'system'

--print("norns module versions: ")
for mod,v in pairs(norns.version) do
    print (mod,v)
end

norns.state.resume()


-- shortcuts
run = norns.script.load
stop = norns.script.cleanup
