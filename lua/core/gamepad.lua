--- gamepad
-- @module gamepad


-- ------------------------------------------------------------------------
-- deps

local hid_events = require "hid_events"


-- ------------------------------------------------------------------------
-- debugging

local debug_level = 0


-- ------------------------------------------------------------------------
-- state

gamepad = {}

-- NB: lots of gamepads like to use their own codes, different from what appears in core/hid_events.lua
gamepad.model = require 'gamepad_model/index'

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
  -- - analog pads, sensor_axis either dpady, dpadx, lefty, leftx, righty, rightx, triggerleft, triggerright
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
-- prevents (expensive) call to `gamepad.code_2_keycode`
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
  }
  return mapping[code]
end

function gamepad.is_axis_keycode_analog(axis_keycode)
  return tab.contains({'ABS_Y', 'ABS_X',
                       'ABS_RY', 'ABS_RX',
                       'ABS_Z', 'ABS_RZ',}, axis_keycode)
end

-- axis keycode -> normalized sensor axis
-- possible return values
-- - dpady / dpadx (dpad)
-- - lefty / leftx (left analog stick)
-- - righty / rightx (right stick)
-- - triggerleft / triggerright (analog shoulder buttons)
function gamepad.axis_keycode_to_sensor_axis(gamepad_conf, axis_keycode)
  return gamepad_conf.axis_mapping[axis_keycode]
end

-- normalized sensor axis -> normalized states
function gamepad.sensor_axis_to_states(sensor_axis)
  local mapping = {
    dpady = {'DPDOWN', 'DPUP'},
    dpadx = {'DPRIGHT', 'DPLEFT'},
    lefty = {'LDOWN', 'LUP'},
    leftx = {'LRIGHT', 'LLEFT'},
    righty = {'RDOWN', 'RUP'},
    rightx = {'RRIGHT', 'RLEFT'},
    triggerleft = {nil, 'TLEFT'},
    triggerright = {nil, 'TRIGHT'},
  }
  return mapping[sensor_axis]
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

function gamepad.register_analog_button_state(sensor_axis, state, inverted, do_log_event)
  local states = gamepad.sensor_axis_to_states(sensor_axis)

  if states == nil then
    return
  end

  local down_state = states[2]

  if state or inverted then
    gamepad.state[down_state] = true
  else
    gamepad.state[down_state] = false
  end
end

function gamepad.register_direction_state(sensor_axis, sign, inverted, do_log_event)
  local states = gamepad.sensor_axis_to_states(sensor_axis)

  if states == nil then
    return
  end

  local s1 = states[1]
  local s2 = states[2]

  if sign == 0 then
    gamepad.state[s1] = false
    gamepad.state[s2] = false
  else
    if inverted then
      sign = sign * - 1
    end
    if sign > 0 then
      gamepad.state[s1] = true
      gamepad.state[s2] = false
      if do_log_event and debug_level >= 1 then print("SENSOR STATE: "..s1) end
    else
      gamepad.state[s1] = false
      gamepad.state[s2] = true
      if do_log_event and debug_level >= 1 then print("SENSOR STATE: "..s2) end
    end
  end
end

function gamepad.register_button_state(button_name, val)
  gamepad.state[button_name] = val
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


-- ------------------------------------------------------------------------
-- analog sensor 2 states

--- Returns true if value for axis is around origin
-- i.e. when joystick / d-pad is not actioned
function gamepad.is_analog_origin(gamepad_conf, axis_keycode, value)
  local origin = gamepad_conf.analog_axis_o[axis_keycode]
  if origin == nil then
    origin = 0
  end
  local noize_margin = gamepad_conf.analog_axis_o_margin[axis_keycode]
  if noize_margin == nil then
    noize_margin = 0
  end
  return ( value >= (origin - noize_margin) and value <= (origin + noize_margin))
end

local function normalized_analog_button_val(gamepad_conf, axis_keycode, val)
  local reso = gamepad_conf.analog_axis_resolution[axis_keycode]

  if gamepad.is_analog_origin(gamepad_conf, axis_keycode, val) then
    val = 0
  end

  local state = false
  local sign = 0
  if val > reso * 2/3 then
    state = true
  end

  if gamepad_conf.axis_invert[axis_keycode] then
    state = not state
  end

  if state then
    sign = 1
  end

  return {val, state, sign}
end

local function normalized_analog_direction_val(gamepad_conf, axis_keycode, val)
  local origin = gamepad_conf.analog_axis_o[axis_keycode]
  local reso = gamepad_conf.analog_axis_resolution[axis_keycode]
  local half_reso = reso / 2

  if gamepad.is_analog_origin(gamepad_conf, axis_keycode, val) then
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

function gamepad.process(guid, typ, code, val, do_log_event)

  local event_code_type
  for k, v in pairs(hid_events.types) do
    if tonumber(v) == typ then
      event_code_type = k
      break
    end
  end

  local gamepad_conf = gamepad.model[guid]
  local gamepad_alias = gamepad_conf.alias

  local do_log_event = gamepad.is_loggable_event(gamepad_conf, event_code_type, code, val)

  if do_log_event and debug_level >= 2 then
    local keycode = gamepad.code_2_keycode(event_code_type, code)
    local msg = "hid.event" .."\t".. gamepad_alias .."\t".. " type: " .. typ .."\t".. " code: " .. code .."\t".. " value: "..val
    if keycode then
      msg = msg .."\t".. " keycode: "..keycode
    end
    print(msg)
  end

  local button_name

  if event_code_type == "EV_ABS" then
    local axis_keycode = gamepad.axis_code_2_keycode(code)
    local sensor_axis = gamepad.axis_keycode_to_sensor_axis(gamepad_conf, axis_keycode)
    local axis = gamepad.sensor_axis_to_axis(sensor_axis)

    local sign

    -- if not recognized axis then
    if not sensor_axis then
      return
    end

    -- TODO: handle relative axes?

    local is_analog = gamepad.is_axis_keycode_analog(axis_keycode)
    local is_dpad = true
    if is_analog and (not gamepad_conf.dpad_is_analog) then
      is_dpad = false
    end

    local is_button = false
    local btn_state = false
    button_name = gamepad.analog_axis_keycode_2_button(gamepad_conf, axis_keycode)
    if button_name then
      is_button = true
    end

    if is_analog then

      if is_button then
        local normalized = normalized_analog_button_val(gamepad_conf, axis_keycode, val)
        val = normalized[1]
        btn_state = normalized[2]
        sign = normalized[3]
      else
        local normalized = normalized_analog_direction_val(gamepad_conf, axis_keycode, val)
        val = normalized[1]
        sign = normalized[2]
      end

      -- first callback -> TODO: kinda wrong to do it before btn states?

    else -- digital
      sign = val
      if sign ~= 0 then
        sign = val < 0 and -1 or 1
      end
    end

    -- register state
    local btn_val = 0
    if is_button then
      gamepad.register_analog_button_state(sensor_axis, btn_state, gamepad_conf.axis_invert[axis_keycode])
      if do_log_event and debug_level >= 1 then print("BUTTON: " .. button_name .. " " .. tostring(btn_state)) end
      gamepad.register_button_state(button_name, btn_state)
    else
      gamepad.register_direction_state(sensor_axis, sign, gamepad_conf.axis_invert[axis_keycode], do_log_event)
    end

    -- callbacks
    -- - gamepad.analog()
    if is_analog and val ~= prev_dir_v[axis_keycode] then
      local reso = gamepad_conf.analog_axis_resolution[axis_keycode]
      local half_reso = reso / 2
      local reported_reso = is_button and reso or half_reso
      local dbg_reso = (val >= 0) and reported_reso or -reported_reso
      if debug_level >= 2 then print("ANALOG: " .. sensor_axis .. " " .. val .. "/" .. dbg_reso) end
      prev_dir_v[axis_keycode] = val
      if _menu.mode then
        if _menu.gamepad_analog then _menu.gamepad_analog(sensor_axis, val, reported_reso) end
      elseif gamepad.analog then gamepad.analog(sensor_axis, val, reported_reso) end
    end

    -- - gamepad.axis() + gamepad.axis() / gamepad.button()
    if sign ~= prev_dir[axis_keycode] then
      prev_dir[axis_keycode] = sign

      if do_log_event and debug_level >= 1 then print("AXIS: " .. sensor_axis .. " " .. sign) end
      gamepad.trigger_axis(sensor_axis, sign)

      if is_button then
        gamepad.trigger_button(button_name, btn_val)
      end

      if is_dpad and axis then
        -- REVIEW: should call again `gamepad.register_direction_state` in that case, right?
        -- for edge case when `gamepad.trigger_dpad` is called by script to simulate user input
        gamepad.trigger_dpad(axis, sign)
      end
    end
  end


  if event_code_type == "EV_KEY" then
  button_name = gamepad.code_2_button(gamepad_conf, code)
  if button_name then

      local btn_state = false
      if val > 0 then
        btn_state = true
      end

      if do_log_event and debug_level >= 1 then print("BUTTON: " .. button_name .. " " .. tostring(btn_state)) end

      gamepad.register_button_state(button_name, btn_state)
      gamepad.trigger_button(button_name, val)

    end
  end
end


-- ------------------------------------------------------------------------
-- debug

--- Predicate that returns true only on non-reset values (i.e. on key/joystick presses)
function gamepad.is_loggable_event(gamepad_conf,event_code_type,code,val)
  if (event_code_type == "EV_KEY") then
    return true
  end
  if event_code_type == "EV_ABS" then
    local axis_keycode = gamepad.code_2_keycode(event_code_type, code)
    if gamepad.is_axis_keycode_analog(axis_keycode) then
      return not gamepad.is_analog_origin(gamepad_conf,axis_keycode,val)
    else
      return (val ~= 0)
    end
  end
end


-- ------------------------------------------------------------------------
-- hid event parsing


--- Returns button name associated w/ (non-axis) key code
function gamepad.code_2_button(gamepad_conf, code)
  if gamepad_conf.button == nil then return end
  local code_2_button = tab.invert(gamepad_conf.button)
  return code_2_button[code]
end

--- Returns button name associated w/ analog axis key code
function gamepad.analog_axis_keycode_2_button(gamepad_conf, axis_keycode)
  if gamepad_conf.analog_button == nil then return end
  local code_2_button = tab.invert(gamepad_conf.analog_button)
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
