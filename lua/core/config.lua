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
local dust = home..'/dust/code/?.lua;'

package.path = sys..core..params..lib..softcut..dust..package.path
-- print('package.path: ' .. package.path)

-- must be done after package path is set
local tu = require 'tabutil'

local _p = {}
_p.home = home
_p.dust = home..'/dust/'
_p.code = _p.dust..'code/'
_p.audio = _p.dust..'audio/'
_p.tape = _p.audio..'tape/'
_p.data = _p.dust..'data/'

_path = tu.readonly{ table = _p }