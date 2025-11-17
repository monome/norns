-- ldoc config
project='norns'
title='norns/docs'
file={
    'lua',
    exclude = {
        'lua/extn',
        'lua/core/gamepad_model',
        'lua/core/keymap',
        'lua/core/menu',
				'lua/lib/test',
				'lua/test'
    },
}
style='!pale'
format='markdown'

-- trying to trick ldoc into processing supercollider classes :S
add_language_extension('sc', 'c')
add_language_extension('c', 'c')
