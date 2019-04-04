-- ldoc config
project='norns'
title='norns/docs'
-- file={'lua', 'matron/src/weaver.c', 'sc/core/Crone.sc'}
--[[file = {
  'lua/core/screen.lua',
  'lua/core/metro.lua'
}
--]]
file = 'lua'
style='!pale'

-- trying to trick ldoc into processing supercollider classes :S
add_language_extension('sc', 'c')
add_language_extension('c', 'c')
