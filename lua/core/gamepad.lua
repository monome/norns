--- gamepad
-- @module gamepad

local tab = require 'tabutil'
local hid_events = require "hid_events"

gamepad = {}

-- NB: lots of gamepads like to use their own codes, different from what appears in core/hid_events.lua
gamepad.model = require 'gamepad_model/index'

--- button states
gamepad.state = {}

local debug_level = 1
-- cache to prevent spamming when val=0 (origin)
local prev_dpad_v = {
  ABS_X = 0,
  ABS_Y = 0,
}

-- clear callbacks
function gamepad.clear()
  gamepad.dpad = function() end
  gamepad.button = function() end
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

function gamepad.process(dev_name,typ,code,val)

  local event_code_type
  for k, v in pairs(hid_events.types) do
    if tonumber(v) == typ then
      event_code_type = k
      break
    end
  end

  local do_log_event = gamepad.is_loggable_event(dev_name, event_code_type, val)

  if do_log_event and debug_level >= 2 then
    local keycode = gamepad.code_2_keycode(event_code_type, code)
    local msg = "hid.event" .."\t".. " type: "..typ .."\t".. " code: ".. code .."\t".. " value: "..val
    if keycode then
      msg = msg .."\t".. " keycode: "..keycode
    end
    print(msg)
  end

  local event_key
  if event_code_type == "EV_ABS" then
    local axis = gamepad.code_2_keycode(event_code_type, code)
    if axis then
      if gamepad.is_dpad_origin(dev_name, val) then
        if axis == 'ABS_Y' then
          gamepad.state.UP = false
          gamepad.state.DOWN = false
        elseif axis == 'ABS_X' then
          gamepad.state.LEFT = false
          gamepad.state.RIGHT = false
        end
        if  prev_dpad_v[axis] ~= 0 then
          prev_dpad_v[axis] = 0
          -- menu dpad
          if _menu.mode then _menu.dpad(axis, val)
            -- script dpad
          elseif gamepad.dpad then gamepad.dpad(axis, 0) end
        end
      else
        local reso = gamepad.model[dev_name].dpad_resolution
        local sign = (val < reso/2) and -1 or 1
        -- TODO: handle inverted axises
        if axis == 'ABS_Y' then
          if gamepad.model[dev_name].dpad_y_invert then
            sign = sign * - 1
          end
          if sign == 1 then
            if do_log_event and debug_level >= 1 then print('UP') end
            gamepad.state.UP = true
            gamepad.state.DOWN = false
          else
            if do_log_event and debug_level >= 1 then print('DOWN') end
            gamepad.state.UP = false
            gamepad.state.DOWN = true
          end
        elseif axis == 'ABS_X' then
          if gamepad.model[dev_name].dpad_x_invert then
            sign = sign * - 1
          end
          if sign == 1 then
            if do_log_event and debug_level >= 1 then print('RIGHT') end
            gamepad.state.RIGHT = true
            gamepad.state.LEFT = false
          else
            if do_log_event and debug_level >= 1 then print('LEFT') end
            gamepad.state.RIGHT = false
            gamepad.state.LEFT = true
          end
        end
        prev_dpad_v[axis] = val
        -- menu dpad
        if _menu.mode then _menu.dpad(axis, val)
          -- script dpad
        elseif gamepad.dpad then gamepad.dpad(axis, val) end
      end
    end
  end

  -- TODO: handle relative axes

  local button_name
  if event_code_type == "EV_KEY" then
    button_name = gamepad.code_2_button(dev_name,code)
    if button_name then

      if do_log_event and debug_level >= 1 then print(button_name) end

      gamepad.state[button_name] = val

      -- menu button
      if _menu.mode then _menu.button(button_name, val)
        -- script button
      elseif gamepad.button then gamepad.button(button_name, val) end
    end
  end
end

--- Predicate that returns true only on non-reset values (i.e. on key/joystick presses)
function gamepad.is_loggable_event(dev_name,event_code_type,val)
  return (event_code_type == "EV_KEY" and val == 1)
    or (event_code_type == "EV_ABS" and not gamepad.is_dpad_origin(dev_name,val))
end

--- Returns true if value for axis is around origin
-- i.e. when joystick / d-pad is not actioned
function gamepad.is_dpad_origin(dev_name,value)
  local margin = gamepad.model[dev_name].dpad_o_margin
  return ( value >= (128 - margin) and value <= (128 + margin) )
end

--- Returns button name associated w/ key code
function gamepad.code_2_button(dev_name,code)
  local code_2_button = tab.invert(gamepad.model[dev_name].button)
  return code_2_button[code]
end

--- Returns event key name associated w/ key code
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
