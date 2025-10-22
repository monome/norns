--- gamepad
--
-- The [norns script reference](https://monome.org/docs/norns/reference/)
-- has [examples for this module](https://monome.org/docs/norns/reference/gamepad).
--
-- @module gamepad

gamepad = {}


-- ------------------------------------------------------------------------
-- deps

local hid_events = require "hid_events"


-- ------------------------------------------------------------------------
-- debugging

local debug_level = 0


-- ------------------------------------------------------------------------
-- consts

gamepad.keys = {
  A       = 'A',
  B       = 'B',
  X       = 'X',
  Y       = 'Y',
  Z       = 'Z',
  START   = 'START',
  SELECT  = 'SELECT',
  L1      = 'L1',
  L2      = 'L2',
  R1      = 'R1',
  R2      = 'R2',
  -- TODO: support stick button presses
}

gamepad.events = {
  EV_KEY  = 'EV_KEY',
  EV_ABS  = 'EV_ABS',
}

gamepad.axes = {
  -- dpad
  dpadx         = 'dpadx',
  dpady         = 'dpady',
  -- analog sticks
  leftx         = 'leftx',
  lefty         = 'lefty',
  rightx        = 'rightx',
  righty        = 'righty',
  -- shoulder triggers (L2/R2 typically)
  lefttrigger   = 'lefttrigger',
  righttrigger  = 'righttrigger',
}


-- ------------------------------------------------------------------------
-- profile db lookup

gamepad.sdl_profile_db_file_path = _path.home .. '/norns/lua/core/gamecontrollerdb.txt'

-- lookup SDL profile database for gamepad matching `guid`
-- returns the raw line
local function lookup_sdl_profile(guid, platform)
  platform = platform or "Linux"
  guid = guid:lower()
  local f = io.open(gamepad.sdl_profile_db_file_path, "r")
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

local function rework_sdl_key_name(phys)
  local mapping = {
    a             = gamepad.keys.A,
    b             = gamepad.keys.B,
    x             = gamepad.keys.X,
    y             = gamepad.keys.Y,
    z             = gamepad.keys.Z,
    back          = gamepad.keys.SELECT,
    start         = gamepad.keys.START,
    leftshoulder  = gamepad.keys.L1,
    rightshoulder = gamepad.keys.R1,
    -- TODO: support stick button presses:
    -- leftstick
    -- rightstick
  }
  return mapping[phys]
end

local function rework_sdl_abs_name(phys)
  if phys == 'dpup' or phys == 'dpdown' then
    return gamepad.axes.dpady
  end
  if phys == 'dpleft' or phys == 'dpright' then
    return gamepad.axes.dpadx
  end

  return gamepad.axes[phys]
end

-- convert from SDL format (bN, aN, aN~, aN+/-, hH.V) to event code
local function sdl_to_ev_code(dev, entry)
  local logical, phys = entry:match("([^:]+):(.+)")
  if not (logical and phys) then
    return
  end

  -- b(uttons) = EV_KEY: b<N>  (N is 0-indexed)
  local n = phys:match("^b(%d+)$")
  if n then
    local idx = tonumber(n) + 1
    return { type = "KEY",
             sdl = entry, sdl_logical = logical,
             logical = rework_sdl_key_name(logical),
             code = dev.codes[hid_events.types.EV_KEY][idx] }
  end

  -- a(xes) = EV_ABS: a<N>, a<N>~, a<N>+, a<N>-
  local mod, a_idx = phys:match("^([~%+%-]?)a(%d+)$")
  if a_idx then
    local idx = tonumber(a_idx) + 1
    local code = dev.abs_codes[idx]
    local e = { type = "ABS",
                sdl = entry, sdl_logical = logical,
                logical = rework_sdl_abs_name(logical),
                code = code,
                abs_keycode = gamepad.axis_code_2_keycode(code) }
    if     mod == "~" or mod == "-" then e.invert = true end
    if mod == "+" or mod == "-" then e.half = true end
    return e
  end

  -- h(ats) = ABS_HAT<H>*: h<H>.<V>  (V: 1=up, 2=right, 4=down, 8=left)
  -- typically dpad, though dpads can sometimes be analog...
  local h_idx, mask = phys:match("^h(%d+)%.(%d+)$")
  if h_idx then
    local h = tonumber(h_idx)
    local v = tonumber(mask)
    local e = { type = "ABS",
                sdl = entry, sdl_logical = logical,
                logical = rework_sdl_abs_name(logical) }
    -- TODO: handle `.invert`
    local hx_code, hy_code = nil, nil
    if h == 0 then
      hx_code, hy_code = hid_events.codes.ABS_HAT0X, hid_events.codes.ABS_HAT0Y
    elseif h == 1 then
      hx_code, hy_code = hid_events.codes.ABS_HAT1X, hid_events.codes.ABS_HAT1Y
    elseif h == 2 then
      hx_code, hy_code = hid_events.codes.ABS_HAT2X, hid_events.codes.ABS_HAT2Y
    elseif h == 3 then
      hx_code, hy_code = hid_events.codes.ABS_HAT3X, hid_events.codes.ABS_HAT3Y
    end
    -- NB: those values are the codes the SDL mapping tells us we'll receive for the current dpad direction
    -- the up/down/left/right in the comments bellow are informative and only really match for standard controllers
    if     v == 1 then e.code, e.value = hy_code, -1  -- up    => Y -1
    elseif v == 2 then e.code, e.value = hx_code,  1  -- right => X +1
    elseif v == 4 then e.code, e.value = hy_code,  1  -- down  => Y +1
    elseif v == 8 then e.code, e.value = hx_code, -1  -- left  => X -1
    else
      print("unexpected gamepad profile entry for ABS/HAT: "+entry)
      return { type = "UNKNOWN", sdl = phys }
    end
    e.abs_keycode = gamepad.axis_code_2_keycode(e.code)

    return e
  end

  -- Fallback
  return { type = "UNKNOWN", sdl = phys }
end


-- generate a gamepad profile (lua table) from a `raw_sdl_profile`
-- the `raw_sdl_profile` is typically retrieved w/ `lookup_sdl_profile`
-- we also pass the `dev` (hid device) to get the analog sensor resolution (`absinfos`), retrived by livevdev in the C layer
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
    dpad_is_analog = false,
    dpad_mapping = {},
  }

  if debug_level >= 1 then print("raw SDL gamepad profile: "..raw_sdl_profile) end

  local guid, name, rest = raw_sdl_profile:match("^([^,]+),([^,]+),(.+)$")
  if not guid then return nil end
  profile.guid = guid
  profile.alias = name

  for entry in rest:gmatch("[^,]+") do
    local mapping = sdl_to_ev_code(dev, entry)
    if mapping then
      if mapping.type == 'KEY' then
        if mapping.logical then
          profile.button[mapping.logical] = mapping.code
        end
      elseif mapping.type == 'ABS' then
        if mapping.abs_keycode and mapping.logical then
          profile.axis_mapping[mapping.abs_keycode] = mapping.logical
          local absinfo = dev.absinfos[mapping.code]
          if gamepad.is_axis_keycode_analog(mapping.abs_keycode) then
            -- NB: SDL reports each axis twice (up/down, left/right) in that case
            if tab.contains({'dpup', 'dpdown', 'dpleft', 'dpright'}, mapping.sdl_logical) then
              profile.dpad_is_analog = true
              if mapping.sdl_logical == 'dpup' then
                profile.axis_invert[mapping.abs_keycode] = not mapping.invert
              elseif mapping.sdl_logical == 'dpdown' then
                profile.axis_invert[mapping.abs_keycode] = mapping.invert
              elseif mapping.sdl_logical == 'dpleft' then
                profile.axis_invert[mapping.abs_keycode] = not mapping.invert
              elseif mapping.sdl_logical == 'dpright' then
                profile.axis_invert[mapping.abs_keycode] = mapping.invert
              end
              profile.analog_axis_resolution[mapping.abs_keycode] = math.abs(absinfo.min) + math.abs(absinfo.max)
              profile.analog_axis_o[mapping.abs_keycode] = (math.abs(absinfo.max) - math.abs(absinfo.min))/2
            else
              profile.axis_invert[mapping.abs_keycode] = mapping.invert and true or false
              profile.analog_axis_resolution[mapping.abs_keycode] = math.abs(absinfo.min) + math.abs(absinfo.max)
              profile.analog_axis_o[mapping.abs_keycode] = (math.abs(absinfo.max) - math.abs(absinfo.min))/2
            end
            if absinfo.flat then
              profile.analog_axis_o_margin[mapping.abs_keycode] = math.max(absinfo.flat or 0, absinfo.fuzz or 0)
            end
          else -- digital, i.e. HAT
            if tab.contains({'dpup', 'dpdown', 'dpleft', 'dpright'}, mapping.sdl_logical) then
              -- NB: we do a dirty trick: multiply orientaiton (`mapping.value`) with raw code value
              -- this allows a fast reverse table lookup in `gamepad.process`
              -- but it makes the format different from other mappings where we use `abs_keycode` (human-readable) instead of `code`
              profile.dpad_mapping[mapping.sdl_logical] = mapping.value * mapping.code
            end
          end
        end
      else
        print("gamepad profile parsing error - couldn't parse "..entry)
      end
    end
  end

  -- NB: if (left|right)trigger but no (left|right)shoulder, map it to L1/R1, else L2/R2
  local ltrigger_axis = tab.invert(profile.axis_mapping)['lefttrigger']
  local rtrigger_axis = tab.invert(profile.axis_mapping)['righttrigger']
  if ltrigger_axis and gamepad.is_axis_keycode_analog(ltrigger_axis) then
    local button = profile.button['L1'] and 'L2' or 'L1'
    profile.analog_button[button] = ltrigger_axis
  end
  if rtrigger_axis and gamepad.is_axis_keycode_analog(rtrigger_axis) then
    local button = profile.button['R1'] and 'R2' or 'R1'
    profile.analog_button[button] = rtrigger_axis
  end

  -- TODO: handle half travel vs resolution better
  -- TODO: handle axis invert for HAT
  -- TODO: use fuzz value from libev

  return profile
end

-- retrieve gamepad profile for `dev`
-- uses the SDL gamepad profile database
gamepad.lookup_profile = function(dev)
  print("looking up if is gamepad for GUID="..dev.guid)
  local raw_sdl_profile = lookup_sdl_profile(dev.guid)
  if not raw_sdl_profile then
    print(" -> not found!")
    return nil
  end

  local profile = parse_sdl_profile(dev, raw_sdl_profile)
  if not profile then
    print("Failed to parse profile for gamepad with GUID="..dev.guid)
    return nil
  end

  return profile
end


-- ------------------------------------------------------------------------
-- state

-- NB: lots of gamepads like to use their own codes, different from what appears in core/hid_events.lua
-- gamepad.model = require 'gamepad_model/index'

--- button states
gamepad.state = {
  DPDOWN = false,
  DPUP = false,
  LDOWN = false,
  LUP = false,
  LLEFT = false,
  LRIGHT = false,
  RDOWN = false,
  RUP = false,
  RLEFT = false,
  RRIGHT = false,
  TLEFT = false,
  TRIGHT = false,
  -- aggregated
  DOWN = false,
  UP = false,
  LEFT = false,
  RIGHT = false,
}

-- cache to prevent spamming when val=0 (origin)
local prev_dir = {
  ABS_X = 0,
  ABS_Y = 0,
  ABS_RX = 0,
  ABS_RY = 0,
  ABS_Z  = 0,
  ABS_RZ = 0,
}
local prev_dir_v = {
  ABS_X = 0,
  ABS_Y = 0,
  ABS_RX = 0,
  ABS_RY = 0,
  ABS_Z = 0,
  ABS_RZ = 0,
}


-- ------------------------------------------------------------------------
-- script lifecycle

-- clear callbacks
function gamepad.clear()
  -- axis callbacks
  -- - directional pad, axis either X or Y
  gamepad.dpad = function(axis, sign) end
  -- - analog pads, sensor_axis either dpady, dpadx, lefty, leftx, righty, rightx, lefttrigger, righttrigger
  gamepad.analog = function(sensor_axis, val, half_reso) end
  -- - all axis input (both digital & analog), value (sign) converted to digital (-1,0,1)
  gamepad.axis = function(sensor_axis, sign) end

  -- button press callback
  gamepad.button = function(button_name, state) end
end

--- macro state shortcuts
function gamepad.up()
  return gamepad.state.DPUP or gamepad.state.LUP end
function gamepad.down()
  return gamepad.state.DPDOWN or gamepad.state.LDOWN end
function gamepad.left()
  return gamepad.state.DPLEFT or gamepad.state.LLEFT end
function gamepad.right()
  return gamepad.state.DPRIGHT or gamepad.state.LRIGHT end


-- ------------------------------------------------------------------------
-- axis lookup / mapping

-- lookup table for keycode -> axis keycode
-- indeed it's expensive to do the lookup through `hid_events.codes`
-- we do have the `gamepad.code_2_keycode` fn for that, but better only use it for debug
function gamepad.axis_code_2_keycode(code)
  local mapping = {
    [0x00] = 'ABS_X',
    [0x01] = 'ABS_Y',
    [0x02] = 'ABS_Z',
    [0x03] = 'ABS_RX',
    [0x04] = 'ABS_RY',
    [0x05] = 'ABS_RZ',
    [0x10] = 'ABS_HAT0X',
    [0x11] = 'ABS_HAT0Y',
    [0x12] = 'ABS_HAT1X',
    [0x13] = 'ABS_HAT1Y',
    [0x14] = 'ABS_HAT2X',
    [0x15] = 'ABS_HAT2Y',
    [0x16] = 'ABS_HAT3X',
    [0x17] = 'ABS_HAT3Y',
  }
  return mapping[code]
end

function gamepad.is_axis_keycode_analog(axis_keycode)
  return tab.contains({ 'ABS_Y',  'ABS_X',
                        'ABS_RY', 'ABS_RX',
                        'ABS_Z',  'ABS_RZ', }, axis_keycode)
end

-- axis keycode -> normalized sensor axis
-- possible return values
-- - dpady / dpadx (dpad)
-- - lefty / leftx (left analog stick)
-- - righty / rightx (right stick)
-- - lefttrigger / righttrigger (analog shoulder buttons)
function gamepad.axis_keycode_to_sensor_axis(gamepad_profile, axis_keycode)
  return gamepad_profile.axis_mapping[axis_keycode]
end

-- normalized sensor axis -> normalized states
function gamepad.sensor_axis_to_states(sensor_axis)
  local mapping = {
    dpady        = { 'DPDOWN',  'DPUP'   },
    dpadx        = { 'DPRIGHT', 'DPLEFT' },
    lefty        = { 'LDOWN',   'LUP'    },
    leftx        = { 'LRIGHT',  'LLEFT'  },
    righty       = { 'RDOWN',   'RUP'    },
    rightx       = { 'RRIGHT',  'RLEFT'  },
    lefttrigger  = { nil,       'TLEFT'  },
    righttrigger = { nil,       'TRIGHT' },
  }
  return mapping[sensor_axis]
end

function gamepad.dpad_dir_to_axis_and_val(dpad_dir)
  local mapping = {
    dpup    = { 'dpady', -1 },
    dpdown  = { 'dpady',  1 },
    dpleft  = { 'dpadx', -1 },
    dpright = { 'dpadx',  1 },
  }
  return mapping[dpad_dir]
end

-- normalized sensor axis -> generic axis
function gamepad.sensor_axis_to_axis(sensor_axis)
  local mapping = {
    dpady = 'Y',
    dpadx = 'X',
    -- lefty = 'Y',
    -- leftx = 'X',
    -- righty = 'RY',
    -- rightx = 'RX',
  }
  return mapping[sensor_axis]
end


-- ------------------------------------------------------------------------
-- state modifiers

function gamepad.register_button_state(button_name, val)
  gamepad.state[button_name] = val
end

function gamepad.register_direction_state(sensor_axis, sign, inverted, do_log_event)
  local states = gamepad.sensor_axis_to_states(sensor_axis)

  if states == nil then
    return
  end

  local s1 = states[1]
  local s2 = states[2]

  if sign == 0 then
    if s1 then gamepad.state[s1] = false end
    gamepad.state[s2] = false
  else
    if inverted then
      sign = sign * - 1
    end
    if sign > 0 then
      if s1 then gamepad.state[s1] = true end
      gamepad.state[s2] = false
      if do_log_event and debug_level >= 1 then print("SENSOR STATE: "..s1) end
    else
      if s1 then gamepad.state[s1] = false end
      gamepad.state[s2] = true
      if do_log_event and debug_level >= 1 then print("SENSOR STATE: "..s2) end
    end
  end
end


-- trigger `gamepad.analog`, but only if value changed from provious occurence
function gamepad.trigger_analog_maybe(gamepad_profile, axis_keycode, sensor_axis, val, is_button)
  if val ~= prev_dir_v[axis_keycode] then
    local reso = gamepad_profile.analog_axis_resolution[axis_keycode]
    local half_reso = reso / 2
    local reported_reso = is_button and reso or half_reso
    local dbg_reso = (val >= 0) and reported_reso or -reported_reso
    if debug_level >= 2 then print("ANALOG: " .. sensor_axis .. " " .. val .. "/" .. dbg_reso) end
    prev_dir_v[axis_keycode] = val

    gamepad.trigger_analog(sensor_axis, val, reported_reso)
  end
end

-- trigger `gamepad.axis`, but only if value changed from provious occurence
function gamepad.trigger_axis_maybe(axis_keycode, sensor_axis, sign)
  if sign ~= prev_dir[axis_keycode] then
    prev_dir[axis_keycode] = sign

    if do_log_event and debug_level >= 1 then print("AXIS: " .. sensor_axis .. " " .. sign) end
    gamepad.trigger_axis(sensor_axis, sign)
  end
end

-- trigger `gamepad.dpad`, but only if value changed from provious occurence
function gamepad.trigger_dpad_maybe(axis_keycode, axis, sign)
  if sign ~= prev_dir[axis_keycode] then
    gamepad.trigger_dpad(axis, sign)
  end
end

function gamepad.trigger_button(button_name, val)
  -- menu button
  if _menu.mode then
    if _menu.gamepad_button then _menu.gamepad_button(button_name, val) end
    -- script button
  elseif gamepad.button then gamepad.button(button_name, val) end
end

function gamepad.trigger_axis(sensor_axis, sign)
  -- menu axis
  if _menu.mode then
    if _menu.gamepad_axis then _menu.gamepad_axis(sensor_axis, sign) end
    -- script axis
  elseif gamepad.axis then gamepad.axis(sensor_axis, sign) end
end

function gamepad.trigger_dpad(axis, sign)
  -- menu dpad
  if _menu.mode then
    if _menu.gamepad_dpad then _menu.gamepad_dpad(axis, sign) end
    -- script dpad
  elseif gamepad.dpad then gamepad.dpad(axis, sign) end
end

function gamepad.trigger_analog(sensor_axis, val, reported_reso)
  -- menu analog
  if _menu.mode then
    if _menu.gamepad_analog then _menu.gamepad_analog(sensor_axis, val, reported_reso) end
    -- script analog
  elseif gamepad.analog then gamepad.analog(sensor_axis, val, reported_reso) end
end


-- ------------------------------------------------------------------------
-- analog sensor 2 states

--- Returns true if value for axis is around origin
-- i.e. when joystick / d-pad is not actioned
function gamepad.is_analog_origin(gamepad_profile, axis_keycode, value)
  local origin = gamepad_profile.analog_axis_o[axis_keycode]
  if origin == nil then
    origin = 0
  end
  local noize_margin = gamepad_profile.analog_axis_o_margin[axis_keycode]
  if noize_margin == nil then
    noize_margin = 0
  end
  return ( value >= (origin - noize_margin) and value <= (origin + noize_margin))
end

local function normalized_analog_button_val(gamepad_profile, axis_keycode, val)
  local reso = gamepad_profile.analog_axis_resolution[axis_keycode]

  if gamepad.is_analog_origin(gamepad_profile, axis_keycode, val) then
    val = 0
  end

  local state = false
  local sign = 0
  if val > reso * 2/3 then
    state = true
  end

  if gamepad_profile.axis_invert[axis_keycode] then
    state = not state
  end

  if state then
    sign = 1
  end

  return {val, state, sign}
end

local function normalized_analog_direction_val(gamepad_profile, axis_keycode, val)
  local origin = gamepad_profile.analog_axis_o[axis_keycode]
  local reso = gamepad_profile.analog_axis_resolution[axis_keycode]
  local half_reso = reso / 2

  if gamepad.is_analog_origin(gamepad_profile, axis_keycode, val) then
    val = 0
  else
    val = val - origin
  end

  if val <= half_reso * 2/3 and val >= - half_reso * 2/3 then
    sign = 0
  else
    sign = val < 0 and -1 or 1
  end

  return {val, sign}
end


-- ------------------------------------------------------------------------
-- incoming events

function gamepad.process(gamepad_profile, typ, code, val, do_log_event)
  local event_code_type = tab.invert(hid_events.types)[typ]
  if event_code_type == nil then return end

  -- if true then
  --   return
  -- end

  local gamepad_alias = gamepad_profile.alias

  local do_log_event = gamepad.is_loggable_event(gamepad_profile, event_code_type, code, val)
  if do_log_event and debug_level >= 2 then
    local keycode = gamepad.code_2_keycode(event_code_type, code)
    local msg = "hid.event" .."\t".. gamepad_alias .."\t".. " type: " .. typ .."(" .. event_code_type .. ")" .."\t".. " code: " .. code .."\t".. " value: "..val
    if keycode then
      msg = msg .."\t".. " keycode: "..keycode
    end
    print(msg)
  end

  local is_event_btn    = gamepad.is_event_btn(gamepad_profile, typ, code, val)
  local is_event_dpad   = gamepad.is_event_dpad(gamepad_profile, typ, code, val)
  local is_event_astick = gamepad.is_event_astick(gamepad_profile, typ, code, val)

  if is_event_dpad or is_event_astick then
    local axis_keycode = gamepad.axis_code_2_keycode(code)
    local sensor_axis = gamepad.axis_keycode_to_sensor_axis(gamepad_profile, axis_keycode)
    if sensor_axis == nil then
      if do_log_event and debug_level >= 2 then
        print("no mapping for axis " .. code .. " (" .. axis_keycode .. ") found in profile")
      end
      return
    end
  end

  if is_event_btn then
    gamepad.process_event_btn(gamepad_profile, typ, code, val)
  elseif is_event_dpad then
    gamepad.process_event_dpad(gamepad_profile, typ, code, val)
  elseif is_event_astick then
    gamepad.process_event_astick(gamepad_profile, typ, code, val)
  end
end


-- ------------------------------------------------------------------------
-- incoming events - buttons (mostly EV_KEY, EV_ABS for L2/R2)

function gamepad.is_event_btn(gamepad_profile, typ, code, val)
  local button_name

  if typ == hid_events.types.EV_ABS then
    local axis_keycode = gamepad.axis_code_2_keycode(code)
    button_name = gamepad.analog_axis_keycode_2_button(gamepad_profile, axis_keycode)
  elseif typ == hid_events.types.EV_KEY then
    button_name = gamepad.code_2_button(gamepad_profile, code)
  end

  return ( button_name ~= nil )
end

function gamepad.process_event_btn(gamepad_profile, typ, code, val)
  local button_name
  local btn_state = false
  local is_analog = false

  local axis_keycode
  local sensor_axis
  local sign

  if typ == hid_events.types.EV_ABS then
    is_analog = true
    axis_keycode = gamepad.axis_code_2_keycode(code)
    sensor_axis = gamepad.axis_keycode_to_sensor_axis(gamepad_profile, axis_keycode)
    button_name = gamepad.analog_axis_keycode_2_button(gamepad_profile, axis_keycode)
    local normalized = normalized_analog_button_val(gamepad_profile, axis_keycode, val)
    val = normalized[1]
    btn_state = normalized[2]
    sign = normalized[3]
  elseif typ == hid_events.types.EV_KEY then
    button_name = gamepad.code_2_button(gamepad_profile, code)
    if val > 0 then
      btn_state = true
    end
  end

  if do_log_event and debug_level >= 1 then print("BUTTON: " .. button_name .. " " .. tostring(btn_state)) end

  -- state update
  gamepad.register_button_state(button_name, btn_state)

  -- callbacks
  if is_analog then
    gamepad.trigger_analog_maybe(gamepad_profile, axis_keycode, sensor_axis, val, true)
    gamepad.trigger_axis_maybe(axis_keycode, sensor_axis, sign)
  end
  gamepad.trigger_button(button_name, val)
end


-- ------------------------------------------------------------------------
-- incoming events - analog sticks (EV_ABS)

function gamepad.is_event_astick(gamepad_profile, typ, code, val)
  if typ ~= hid_events.types.EV_ABS then return false end

  local axis_keycode = gamepad.axis_code_2_keycode(code)
  local is_analog = gamepad.is_axis_keycode_analog(axis_keycode)
  if not is_analog then return end

  local sensor_axis = gamepad.axis_keycode_to_sensor_axis(gamepad_profile, axis_keycode)
  return not tab.contains({gamepad.axes.dpadx, gamepad.axes.dpady}, sensor_axis)
end

function gamepad.process_event_astick(gamepad_profile, typ, code, val)
  local axis_keycode = gamepad.axis_code_2_keycode(code)
  local sensor_axis = gamepad.axis_keycode_to_sensor_axis(gamepad_profile, axis_keycode)

  local normalized = normalized_analog_direction_val(gamepad_profile, axis_keycode, val)
  val = normalized[1]
  sign = normalized[2]

  -- state update
  gamepad.register_direction_state(sensor_axis, sign, gamepad_profile.axis_invert[axis_keycode], do_log_event)

  -- callbacks
  gamepad.trigger_analog_maybe(gamepad_profile, axis_keycode, sensor_axis, val)
  gamepad.trigger_axis_maybe(axis_keycode, sensor_axis, sign)
end


-- ------------------------------------------------------------------------
-- incoming events - dpad (EV_ABS)

function gamepad.is_event_dpad(gamepad_profile, typ, code, val)
  -- NB: we have to pass `val` which is counter-intuitive but that due to how we encode `gamepad_profile.dpad_mapping` (with a slight optimization)

  -- REVIEW: do we have use-cases where a dpad is mapped to `EV_KEY`?
  if typ ~= hid_events.types.EV_ABS then return false end

  local axis_keycode = gamepad.axis_code_2_keycode(code)
  local is_analog = gamepad.is_axis_keycode_analog(axis_keycode)

  if is_analog then -- case `ABS_R?(X|Y|Z)`
    if not gamepad_profile.dpad_is_analog then return end
    local sensor_axis = gamepad.axis_keycode_to_sensor_axis(profile, axis_keycode)
    return tab.contains({gamepad.axes.dpadx, gamepad.axes.dpady}, sensor_axis)
  else -- case `ABS_HAT*`
    -- NB: we assume all digital ABS (`ABS_HAT*`) events are mapped to the dpad
    if gamepad_profile.dpad_is_analog then return end
    local dpad_dir = tab.invert(gamepad_profile.dpad_mapping)[code * (val ~= 0 and val or 1)]
    return ( dpad_dir ~= nil )
  end

  return false
end

function gamepad.process_event_dpad(gamepad_profile, typ, code, val)
  local axis_keycode = gamepad.axis_code_2_keycode(code)
  local sensor_axis = gamepad.axis_keycode_to_sensor_axis(gamepad_profile, axis_keycode)
  local is_analog = gamepad.is_axis_keycode_analog(axis_keycode)

  if is_analog then -- case `ABS_R?(X|Y|Z)`
    if not gamepad_profile.dpad_is_analog then return end

    -- REVIEW: replace by call to `gamepad.process_event_astick`?

    local normalized = normalized_analog_direction_val(gamepad_profile, axis_keycode, val)
    val = normalized[1]
    sign = normalized[2]
  else -- case `ABS_HAT*`
    if gamepad_profile.dpad_is_analog then return end
    local dpad_dir = tab.invert(gamepad_profile.dpad_mapping)[code * (val ~= 0 and val or 1)]

    local remapped = gamepad.dpad_dir_to_axis_and_val(dpad_dir)
    sensor_axis = remapped[1]
    val  = val ~= 0 and remapped[2] or 0

    sign = val
    if sign ~= 0 then
      sign = val < 0 and -1 or 1
    end
  end

  -- state update
  gamepad.register_direction_state(sensor_axis, sign, gamepad_profile.axis_invert[axis_keycode], do_log_event)

  -- callbacks
  if is_analog then
    gamepad.trigger_analog_maybe(axis_keycode, sensor_axis, val)
  end
  gamepad.trigger_dpad_maybe(axis_keycode, sensor_axis, sign)
  gamepad.trigger_axis_maybe(axis_keycode, sensor_axis, sign)
end

-- ------------------------------------------------------------------------
-- debug

--- Predicate that returns true only on non-reset values (i.e. on key/joystick presses)
function gamepad.is_loggable_event(gamepad_profile, event_code_type, code, val)
  if event_code_type == gamepad.events.EV_KEY then
    return true
  end
  if event_code_type == gamepad.events.EV_ABS then
    local axis_keycode = gamepad.axis_code_2_keycode(code)
    if gamepad.is_axis_keycode_analog(axis_keycode) then
      return not gamepad.is_analog_origin(gamepad_profile, axis_keycode, val)
    else
      return (val ~= 0)
    end
  end
end


-- ------------------------------------------------------------------------
-- hid event parsing

--- Returns button name associated w/ (non-axis) key code
function gamepad.code_2_button(gamepad_profile, code)
  if gamepad_profile.button == nil then return end
  local code_2_button = tab.invert(gamepad_profile.button)
  return code_2_button[code]
end

--- Returns button name associated w/ analog axis key code
function gamepad.analog_axis_keycode_2_button(gamepad_profile, axis_keycode)
  if gamepad_profile.analog_button == nil then return end
  local code_2_button = tab.invert(gamepad_profile.analog_button)
  return code_2_button[axis_keycode]
end

--- Returns event key name associated w/ key code
-- this is not lightweight so should only be used in debug statements
function gamepad.code_2_keycode(event_code_type, code)
  for k, v in pairs(hid_events.codes) do
    if tonumber(v) == code then
      if util.string_starts(k, gamepad.event_code_type_2_key_prfx(event_code_type)) then
        return k
      end
    end
  end
end

function gamepad.event_code_type_2_key_prfx(event_code_type)
  return string.sub(event_code_type, -3)
end


-- ------------------------------------------------------------------------

return gamepad
