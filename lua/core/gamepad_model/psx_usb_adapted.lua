
local g = {
  hid_name = 'DWC OTG Controller',
  alias = 'PSX (adapted)',
  button = {
    A = 0x121,
    B = 0x120,
    X = 0x123,
    Y = 0x122,
    L1 = 0x124,
    R1 = 0x125,
    L2 = 0x126,
    R2 = 0x127,
    SELECT = 0x128,
    START = 0x129,
  },

  dpad_o_margin = 28, -- margin to handle noise on center position
  dpad_resolution = 256,
  dpad_invert = {
    Y = false,
    X = true,
    RZ = true,
    Z = false,
  },
}

return g
