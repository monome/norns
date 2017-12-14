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

-- user startup script
--require 'first'

norns.state.resume()


grid = require 'grid'
metro = require 'metro'
e = require 'engine'

-- shortcuts
run = norns.script.load
stop = norns.script.cleanup
