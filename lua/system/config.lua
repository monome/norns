-- norns configuration

-- add some stuff to package.path
-- this consists of search patterns lua will use for require('foo')

local home = os.getenv('HOME')
local norns = home..'/norns/lua'
local sys = norns..'/?.lua;'
local lib = norns..'/lib/?.lua;'
local engine = norns..'/engine/?.lua;'
--local lib = home..'/dust/lib/lua/?.lua;'

package.path = sys..lib..engine..package.path
-- print('package.path: ' .. package.path)

-- full path to directory containing user scripts
dust_dir = home..'/dust/'
data_dir = home..'/dust/data/'
audio_dir = home..'/dust/audio/'
home_dir = home

