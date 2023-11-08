function init_norns()
  _boot.add_io('keys:gpio',    {dev='/dev/input/by-path/platform-keys-event'})
  _boot.add_io('enc:gpio',     {dev='/dev/input/by-path/platform-soc:knob1-event', index=1})
  _boot.add_io('enc:gpio',     {dev='/dev/input/by-path/platform-soc:knob2-event', index=2})
  _boot.add_io('enc:gpio',     {dev='/dev/input/by-path/platform-soc:knob3-event', index=3})
end

function init_desktop()
  -- desktop window
  _boot.add_io('screen:sdl', {})
  -- _boot.add_io('input:sdl', {})

  -- i/o via maiden
  -- _boot.add_io('screen:json', {})
  -- _boot.input_add('web_input', 'json', {})
end

init_norns()

