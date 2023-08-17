-- Retrolink B00GWKL3Y4 ((S)NES-style)
-- also sold by under other brands: iNNext, Elecom...

return {
  alias = "Retro Controller",
  guid = "03000000790000001100000010010000",
  analog_axis_o = {
    ABS_X = 127,
    ABS_Y = 127
  },
  analog_axis_o_margin = {},
  analog_axis_resolution = {
    ABS_X = 256,
    ABS_Y = 256
  },
  axis_invert = {
    ABS_X = false,
    ABS_Y = false
  },
  axis_mapping = {
    ABS_X = "dpadx",
    ABS_Y = "dpady"
  },
  button = {
    A = 289,
    B = 290,
    SELECT = 296,
    START = 297
  },
  dpad_is_analog = true,
  hid_name = "USB Gamepad "
}
