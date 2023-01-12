return {
  alias = "Xbox 360 Controller",
  guid  = "030000005e0400008e02000014010000",
  analog_axis_o = {
    ABS_RX = 0,
    ABS_RY = 0,
    ABS_X = 0,
    ABS_Y = 0,
    ABS_Z = 0,
    ABS_RZ = 0
  },
  analog_axis_o_margin = {},
  analog_axis_resolution = {
    ABS_RX = 65535,
    ABS_RY = 65535,
    ABS_X = 65535,
    ABS_Y = 65535,
    ABS_Z = 255,
    ABS_RZ = 255
  },
  axis_invert = {
    ABS_HAT0X = false,
    ABS_HAT0Y = false,
    ABS_RX = false,
    ABS_RY = false,
    ABS_X = false,
    ABS_Y = false
  },
  axis_mapping = {
    ABS_HAT0X = "dpadx",
    ABS_HAT0Y = "dpady",
    ABS_RX = "rightx",
    ABS_RY = "righty",
    ABS_X = "leftx",
    ABS_Y = "lefty",
    ABS_Z = "triggerleft",
    ABS_RZ = "triggerright"
  },
  button = {
    A = 305,
    B = 304,
    L1 = 310,
    R1 = 311,
    SELECT = 314,
    START = 315,
    X = 308,
    Y = 307
  },
  analog_button = {
    L2 = "ABS_Z",
    R2 = "ABS_RZ",
  },
  dpad_is_analog = false,
  hid_name = "Controller"
}
