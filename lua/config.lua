-- add some stuff to package.path
-- this consists of search patterns lua will use for require('foo')

local home = os.getenv('HOME')
local norns = home..'/norns/lua'
local sys = norns..'/?.lua;'
local script = home..'/dust/?.lua;'

package.path = sys..script..package.path

-- full path to directory containing user scripts
script_dir = home..'/dust/lua/'
data_dir = home..'/dust/data/'

require 'norns'

-- print('package.path: ' .. package.path)
