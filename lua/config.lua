-- add some stuff to package.path
-- this consists of search patterns lua will use for require('foo')

local home = os.getenv('HOME')
local norns = home..'/norns/lua'
local sys = norns..'/?.lua;'
local script = norns..'/scripts/?.lua;'

package.path = sys..script..package.path

-- full path to directory containing user scripts
script_dir = norns..'/scripts/'

-- print('package.path: ' .. package.path)
