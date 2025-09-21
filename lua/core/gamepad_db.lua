
local GamepadDb = {}

GamepadDb.file_path = _path.home .. '/norns/lua/core/gamecontrollerdb.txt'

local hid_events = require "hid_events"

local function lookup_sdl_profile(guid, platform)
  platform = platform or "Linux"
  guid = guid:lower()
  local f = io.open(GamepadDb.file_path, "r")
  if not f then
    print("failed to open gamepad db at path: "..GamepadDb.file_path)
    return nil
  end
  local raw_sdl_profile
  for line in f:lines() do
    if line:sub(1,1) ~= "#" and line:find("platform:" .. platform) and line:find(guid) then
      raw_sdl_profile = line
      break
    end
  end
  f:close()
  return raw_sdl_profile
end

-- lookup table for keycode -> axis keycode
local function axis_code_2_keycode(code)
  local mapping = {
    [0x00] = 'ABS_X',
    [0x01] = 'ABS_Y',
    [0x02] = 'ABS_Z',
    [0x03] = 'ABS_RX',
    [0x04] = 'ABS_RY',
    [0x05] = 'ABS_RZ',
    [0x10] = 'ABS_HAT0X',
    [0x11] = 'ABS_HAT0Y',
  }
  return mapping[code]
end

local function is_axis_keycode_analog(axis_keycode)
  return tab.contains({'ABS_Y', 'ABS_X',
                       'ABS_RY', 'ABS_RX',
                       'ABS_Z', 'ABS_RZ',}, axis_keycode)
end


local function rework_sdl_key_name(phys)
  local mapping = {
    a             = 'A',
    b             = 'B',
    x             = 'X',
    y             = 'Y',
    z             = 'Z',
    back          = 'SELECT',
    start         = 'START',
    leftshoulder  = 'L1',
    rightshoulder = 'R1',
    -- leftstick
    -- rightstick
  }
  return mapping[phys]
end

local function rework_sdl_abs_name(phys)
  local mapping = {
    leftx        = 'leftx',
    lefty        = 'lefty',
    rightx       = 'rightx',
    righty       = 'righty',
    lefttrigger  = 'lefttrigger',
    righttrigger = 'righttrigger',

    -- FIXME: if SDL has them split in 4 it must mean that some
    dpdup        = 'dpady',
    dpdown       = 'dpady',
    dpleft       = 'dpadx',
    dpright      = 'dpadx',
  }
  return mapping[phys]
end


-- convert from SDL format (bN, aN, aN~, aN+/-, hH.V) to event code
local function sdl_to_ev_code(dev, entry)
  local logical, phys = entry:match("([^:]+):(.+)")
  if not (logical and phys) then
    return
  end

  -- b(uttons) = EV_KEY: bN  (N is 0-based)
  local n = phys:match("^b(%d+)$")
  if n then
    local idx = tonumber(n) + 1
    return { type = "KEY",
             sdl = entry, sdl_logical = logical,
             logical = rework_sdl_key_name(logical),
             code = dev.codes[hid_events.types.EV_KEY][idx] }
  end

  -- a(xes) = EV_ABS: aN, aN~, aN+, aN-
  local mod, a_idx = phys:match("^([~%+%-]?)a(%d+)$")
  if a_idx then
    local idx = tonumber(a_idx) + 1
    local code = dev.abs_codes[idx]
    local e = { type = "ABS",
                sdl = entry, sdl_logical = logical,
                logical = rework_sdl_abs_name(logical),
                code = code,
                abs_keycode = axis_code_2_keycode(code) }
    if     mod == "~" or mod == "-" then e.invert = true end
    if mod == "+" or mod == "-" then e.half = true end
    return e
  end

  -- h(ats) = ABS_HAT0*: hH.V  (V: 1=up, 2=right, 4=down, 8=left)
  -- typically dpad, though dpads can sometimes be analog...
  local h_idx, mask = phys:match("^h(%d+)%.(%d+)$")
  if h_idx then
    local h = tonumber(h_idx)
    local v = tonumber(mask)
    local e = { type = "ABS",
                sdl = entry, sdl_logical = logical,
                logical = rework_sdl_abs_name(logical) }
    -- TODO: handle `.invert`
    if     v == 1 then e.code, e.value = hid_events.codes.ABS_HAT0Y, -1  -- up    => Y -1
    elseif v == 2 then e.code, e.value = hid_events.codes.ABS_HAT0X,  1  -- right => X +1
    elseif v == 4 then e.code, e.value = hid_events.codes.ABS_HAT0Y,  1  -- down  => Y +1
    elseif v == 8 then e.code, e.value = hid_events.codes.ABS_HAT0X, -1  -- left  => X -1
    else               e.mask = v                       -- unexpected bitmask
    end
    e.abs_keycode = axis_code_2_keycode(e.code)

    return e
  end

  -- Fallback
  return { type = "UNKNOWN", sdl = phys }
end


local function parse_sdl_profile(dev, raw_sdl_profile)
  local profile = {
    hid_name = dev.name,
    button = {},
    analog_button = {},
    axis_mapping = {},
    analog_axis_o = {},
    analog_axis_o_margin = {},
    analog_axis_resolution = {},
    axis_invert = {},
    dpad_is_analog = true,
  }

  local guid, name, rest = raw_sdl_profile:match("^([^,]+),([^,]+),(.+)$")
  if not guid then return nil end
  profile.guid = guid
  profile.alias = name

  for entry in rest:gmatch("[^,]+") do
    local mapping = sdl_to_ev_code(dev, entry)
    if mapping then
      -- print("----------------------")
      -- tab.print(mapping)
      if mapping.type == 'KEY' then
        if mapping.logical then
          profile.button[mapping.logical] = mapping.code
        end
      elseif mapping.type == 'ABS' then
        local abs_keycode = axis_code_2_keycode(mapping.code)
        if mapping.abs_keycode and mapping.logical then
          profile.axis_mapping[mapping.abs_keycode] = mapping.logical
          profile.axis_invert[mapping.abs_keycode] = mapping.invert and true or false

          local absinfo = dev.absinfos[mapping.code]
          if absinfo then -- if analog
            if tab.contains({'dpdup', 'dpdown', 'dpleft', 'dpright'}, mapping.sdl_logical) then
              profile.dpad_is_analog = true
            end
            profile.analog_axis_resolution[abs_keycode] = math.abs(absinfo.min) + math.abs(absinfo.max)
            profile.analog_axis_o[abs_keycode] = (math.abs(absinfo.max) - math.abs(absinfo.min))/2
            if absinfo.flat then
              profile.analog_axis_o_margin[abs_keycode] = math.max(absinfo.flat or 0, absinfo.fuzz or 0)
            end
          end
        end
      end
    end
  end

  -- NB: if (left|right)trigger but no (left|right)shoulder, map it to L1/R1, else L2/R2
  local ltrigger_axis = tab.invert(profile.axis_mapping)['lefttrigger']
  local rtrigger_axis = tab.invert(profile.axis_mapping)['righttrigger']
  if ltrigger_axis and is_axis_keycode_analog(ltrigger_axis) then
    local button = profile.button['L1'] and 'L2' or 'L1'
    profile.analog_button[button] = ltrigger_axis
  end
  if rtrigger_axis and is_axis_keycode_analog(rtrigger_axis) then
    local button = profile.button['R1'] and 'R2' or 'R1'
    profile.analog_button[button] = rtrigger_axis
  end

  -- TODO: handle half travel vs resolution better
  -- TODO: handle HAT as 4separate axis like SDL does
  -- TODO: handle axis invert for HAT
  -- TODO: use fuzz value from libev

  return profile
end

GamepadDb.lookup = function(dev)
  local raw_sdl_profile = lookup_sdl_profile(dev.guid)
  if not raw_sdl_profile then
    return nil
  end

  local profile = parse_sdl_profile(dev, raw_sdl_profile)
  if not profile then
    print("Failed to parse profile for gamepad with GUID="..dev.guid)
    return nil
  end

  return profile
end

return GamepadDb
