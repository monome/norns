-- norns configuration

-- add some stuff to package.path
-- this consists of search patterns lua will use for require('foo')

local home = os.getenv('HOME')
local norns = home..'/norns/lua'
local sys = norns..'/?.lua;'
local core = norns..'/core/?.lua;'
local params = norns..'/core/params/?.lua;'
local lib = norns..'/lib/?.lua;'
local softcut = norns..'/softcut/?.lua;'
local dust = home..'/dust/scripts/?.lua;'

package.path = sys..core..params..lib..softcut..dust..package.path
-- print('package.path: ' .. package.path)

_path = {}
_path.home = home
_path.dust = home..'/dust/'
_path.scripts = _path.dust..'scripts/'
_path.audio = _path.dust..'audio/'
_path.data = _path.dust..'data/'
