-- add some stuff to package.path
-- this consists of search patterns lua will use for require('foo')

local home = os.getenv('HOME')
local norns = home..'/norns/lua'
local sys = norns..'/sys/?.lua;'
local script = norns..'/scripts/?.lua;'
local state = norns..'/state/?.lua;'
local audio = norns..'/audio/?.lua;'

package.path = sys..script..state..audio..package.path

print('package.path: ' .. package.path)
