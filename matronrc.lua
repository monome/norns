-- norns / Norns shield
_boot.screen_add('hw_screen', 'fbdev', {dev='/dev/fb0'})
_boot.input_add('hw_keys', 'gpio_keys', {
   dev='/dev/input/by-path/platform-keys-event',
})
_boot.input_add('hw_enc1', 'gpio_enc', {
   index=1,
   dev='/dev/input/by-path/platform-soc:knob1-event',
})
_boot.input_add('hw_enc2', 'gpio_enc', {
   index=2,
   dev='/dev/input/by-path/platform-soc:knob2-event',
})
_boot.input_add('hw_enc3', 'gpio_enc', {
   index=3,
   dev='/dev/input/by-path/platform-soc:knob3-event',
})

-- desktop window
-- _boot.screen_add('desktop_screen', 'sdl', {})
-- _boot.input_add('desktop_input' 'keys_kb', {})

-- i/o via maiden
-- _boot.screen_add('web_screen', 'json', {})
-- _boot.input_add('web_input', 'json', {})