--
local hid_events = require 'hid_events'
local tab = require 'tabutil'

local gamepad = require 'gamepad'


local HidDeviceClass = {}

HidDeviceClass.is_ascii_keyboard = function(device)
  local types_inv = tab.invert(device.types)
  local type_key = hid_events.types.EV_KEY
  -- an ascii keyboard should support key events
  if types_inv[type_key] == nil then
        return false
    end
    local key_codes_inv = tab.invert(device.codes[type_key])
    -- hacky magic numbers;
    -- codes for ascii key events happen to be in this range:
    for i=1,58 do
        if key_codes_inv[i] == nil then
            return false
        end
    end
    return true
end

HidDeviceClass.is_mouse = function(device)
    local types_inv = tab.invert(device.types)
    local type_key = hid_events.types.EV_KEY
    local type_rel = hid_events.types.EV_REL
    -- a mouse should support relative X/Y movement events,
    -- and at least 1 button
    if types_inv[type_key] == nil then
        return false
    end
    if types_inv[type_rel] == nil then
        return false
    end
    local key_codes_inv = tab.invert(device.codes[type_key])

    if key_codes_inv[hid_events.codes.BTN_MOUSE] == nil then
        return false
    end

    local rel_codes_inv = tab.invert(device.codes[type_rel])

    if rel_codes_inv[hid_events.codes.REL_X] == nil then
        return false
    end
    if rel_codes_inv[hid_events.codes.REL_Y] == nil then
        return false
    end
    return true
end

HidDeviceClass.lookup_gamepad_profile = function(device)
  if device.guid == nil then
    return false
  end

  return gamepad.lookup_profile(device)
end

HidDeviceClass.is_tablet = function(device)
  -- TODO
end

-- ... other TODO?


return HidDeviceClass
