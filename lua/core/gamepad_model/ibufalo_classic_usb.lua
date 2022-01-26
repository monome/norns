
local g = {
  hid_name = 'USB,2-axis 8-button gamepad  ',
  alias = 'iBUFFALO Classic USB Gamepad',

  button = {
    A = 288,
    B = 289,
    X = 290,
    Y = 291,
    L1 = 292,
    R1 = 293,
    SELECT = 294,
    START = 295,
  },

  axis_invert = {
    Y = false,
    X = true,
  },

  dpad_is_analog = true,

  analog_axis_resolution = 256,
  -- margin to handle noise on center position
  analog_axis_o_margin = {
    X = 2,
    Y = 2,
  },
}

return g
