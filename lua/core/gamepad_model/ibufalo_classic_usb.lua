
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

  dpad_o_margin = 2, -- margin to handle noise on center position
  dpad_resolution = 256,
  dpad_invert = {
    Y = true,
    X = false,
  },
}

return g
