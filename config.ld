-- ldoc config
project='norns'
title='norns/docs'
-- file={'lua', 'matron/src/weaver.c', 'sc/core/Crone.sc'}
file = 'lua'
style='!pale'

-- trying to trick ldoc into processing supercollider classes :S
add_language_extension('sc', 'c')
add_language_extension('c', 'c')
