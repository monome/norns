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

-- globals
grid = require 'grid'
metro = require 'metro'
e = require 'engine'

-- user startup script
--require 'first' 

-- shortcuts
run = norns.script.load
stop = norns.script.cleanup

-- helper
math.randomseed(os.time())

-- resume last loaded script
norns.state.resume()
