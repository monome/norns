-- add some stuff to package.path
-- this consists of search patterns lua will use for require('foo')

local home = os.getenv('HOME')
local norns = home..'/norns/lua'
local sys = norns..'/?.lua;'
local lib = home..'/dust/lib/lua/?.lua;'

package.path = sys..lib..package.path

-- full path to directory containing user scripts
script_dir = home..'/dust/script/'
data_dir = home..'/dust/data/'
audio_dir = home..'/dust/audio/'
home_dir = home

require 'norns'

-- print('package.path: ' .. package.path)
