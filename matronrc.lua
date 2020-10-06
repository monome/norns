function init_norns()
  _boot.add_io('screen:fbdev', {dev='/dev/fb0'})
  _boot.add_io('keys:gpio',    {dev='/dev/input/by-path/platform-keys-event'})
  _boot.add_io('enc:gpio',     {dev='/dev/input/by-path/platform-soc:knob1-event', index=1})
  _boot.add_io('enc:gpio',     {dev='/dev/input/by-path/platform-soc:knob2-event', index=2})
  _boot.add_io('enc:gpio',     {dev='/dev/input/by-path/platform-soc:knob3-event', index=3})

  -- _boot.screen_add('hw_screen', 'fbdev', {dev='/dev/fb0'})
  -- _boot.input_add('hw_keys', 'gpio_keys', {
  --   dev='/dev/input/by-path/platform-keys-event',
  -- })
  -- _boot.input_add('hw_enc1', 'gpio_enc', {
  --   index=1,
  --   dev='/dev/input/by-path/platform-soc:knob1-event',
  -- })
  -- _boot.input_add('hw_enc2', 'gpio_enc', {
  --   index=2,
  --   dev='/dev/input/by-path/platform-soc:knob2-event',
  -- })
  -- _boot.input_add('hw_enc3', 'gpio_enc', {
  --   index=3,
  --   dev='/dev/input/by-path/platform-soc:knob3-event',
  -- })
end

function init_desktop()
  -- desktop window
  _boot.add_io('screen:sdl', {})
  -- _boot.screen_add('desktop_screen', 'sdl', {})
  -- _boot.input_add('desktop_input' 'keys_kb', {})

  -- i/o via maiden
  -- _boot.screen_add('web_screen', 'json', {})
  -- _boot.input_add('web_input', 'json', {})
end

init_desktop()