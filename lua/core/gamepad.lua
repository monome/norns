--- gamepad
-- @module gamepad


-- ------------------------------------------------------------------------
-- deps

local hid_events = require "hid_events"


-- ------------------------------------------------------------------------
-- static conf

local debug_level = 1


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
  gamepad.astick = function(sensor_axis, val, half_reso) end
  -- - all axis input (both digital & analog), value (sign) converted to digital (-1,0,1)
  gamepad.axis = function(sensor_axis, sign) end

  -- button press callback
  gamepad.button = function(button_name, state) end
end

--- states shortcuts
function gamepad.up()
  return gamepad.state.UP end
function gamepad.down()
  return gamepad.state.DOWN end
function gamepad.left()
  return gamepad.state.LEFT end
function gamepad.right()
  return gamepad.state.RIGHT end


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
    dpadx = {'DPLEFT', 'DPRIGHT'},
    lefty = {'LDOWN', 'LUP'},
    leftx = {'LLEFT', 'LRIGHT'},
    righty = {'RDOWN', 'RUP'},
    rightx = {'RLEFT', 'RRIGHT'},
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

function gamepad.register_direction_state(guid, axis_evt, sign, do_log_event)
  local sensor_axis = gamepad.axis_keycode_to_sensor_axis(gamepad.model[guid], axis_evt)
  local states = gamepad.sensor_axis_to_states(sensor_axis)
  local s1 = states[1]
  local s2 = states[2]

  if states then
    if sign == 0 then
      gamepad.state[s1] = false
      gamepad.state[s2] = false
      if tab.contains({'dpady', 'lefty'}, sensor_axis) then
        gamepad.state.DOWN = false
        gamepad.state.UP = false
      elseif tab.contains({'dpadx', 'leftx'}, sensor_axis) then
        gamepad.state.LEFT = false
        gamepad.state.RIGHT = false
      end
    else
      if gamepad.model[guid].axis_invert[axis_evt] then
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

    -- aggregated states
    if gamepad.state.DPDOWN or gamepad.state.LDOWN then
      gamepad.state.DOWN = true
      gamepad.state.UP = false
      if do_log_event and debug_level >= 1 then print("AXIS STATE: DOWN") end
    elseif gamepad.state.DPUP or gamepad.state.LUP then
      gamepad.state.DOWN = false
      gamepad.state.UP = true
      if do_log_event and debug_level >= 1 then print("AXIS STATE: UP") end
    elseif gamepad.state.DPLEFT or gamepad.state.LLEFT then
      gamepad.state.LEFT = true
      gamepad.state.RIGHT = false
      if do_log_event and debug_level >= 1 then print("AXIS STATE: LEFT") end
    elseif gamepad.state.DPRIGHT or gamepad.state.LRIGHT then
      gamepad.state.LEFT = false
      gamepad.state.RIGHT = true
      if do_log_event and debug_level >= 1 then print("AXIS STATE: RIGHT") end
    end
  end
end


function gamepad.process(guid, typ, code, val)

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

  local event_key
  if event_code_type == "EV_ABS" then
    local axis_keycode = gamepad.axis_code_2_keycode(code)
    local sensor_axis = gamepad.axis_keycode_to_sensor_axis(gamepad_conf, axis_keycode)
    local axis = gamepad.sensor_axis_to_axis(sensor_axis)

    local sign = val

    -- if axis then
    if sensor_axis then
      local is_analog = gamepad.is_axis_keycode_analog(axis_keycode)
      local is_dpad = true
      if is_analog and (not gamepad_conf.dpad_is_analog) then
        is_dpad = false
      end

      if is_analog then
        local origin = gamepad_conf.analog_axis_o[axis_keycode]
        local reso = gamepad_conf.analog_axis_resolution[axis_keycode]
        local half_reso = reso / 2

        if gamepad.is_analog_origin(gamepad_conf, axis_keycode, val) then
          val = 0
        else
          val = val - origin
        end

        if val ~= prev_dir_v[axis_keycode] then
          prev_dir_v[axis_keycode] = val
          if (not is_dpad) and gamepad.astick then gamepad.astick(sensor_axis, val, half_reso) end
        end

        -- analog value count as a direction change IIF value > 2/3 of resolution
        if val <= half_reso * 2/3 and val >= - half_reso * 2/3 then
          sign = 0
        else
          sign = val < 0 and -1 or 1
        end
      else -- digital
        if sign ~= 0 then
          sign = val < 0 and -1 or 1
        end
      end

      gamepad.register_direction_state(guid, axis_keycode, sign, do_log_event)

      if sign ~= prev_dir[axis_keycode] then
        prev_dir[axis_keycode] = sign

        -- menu axis
        if _menu.mode and _menu.axis then _menu.axis(sensor_axis, sign)
          -- script axis
        elseif gamepad.axis then gamepad.axis(sensor_axis, sign) end

        if is_dpad and axis then
          -- menu dpad
          if _menu.mode and _menu.dpad then _menu.dpad(axis, sign)
            -- script dpad
          elseif gamepad.dpad then gamepad.dpad(axis, sign) end
        end
      end

    end
  end

  -- TODO: handle relative axes

  local button_name
  if event_code_type == "EV_KEY" then
    button_name = gamepad.code_2_button(gamepad_conf, code)
    if button_name then

      if do_log_event and debug_level >= 1 then print("BUTTON:"..button_name) end

      gamepad.state[button_name] = val

      -- menu button
      if _menu.mode then _menu.button(button_name, val)
        -- script button
      elseif gamepad.button then gamepad.button(button_name, val) end
    end
  end
end

--- Predicate that returns true only on non-reset values (i.e. on key/joystick presses)
function gamepad.is_loggable_event(gamepad_conf,event_code_type,code,val)
  if (event_code_type == "EV_KEY" and val == 1) then
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

--- Returns true if value for axis is around origin
-- i.e. when joystick / d-pad is not actioned
function gamepad.is_analog_origin(gamepad_conf,axis_keycode,value)
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

--- Returns button name associated w/ key code
function gamepad.code_2_button(gamepad_conf,code)
  local code_2_button = tab.invert(gamepad_conf.button)
  return code_2_button[code]
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




return gamepad
