--- Formatters
-- @module formatters
-- @alias Formatters
local Formatters = {}

local function format(param, value, units)
  return value.." "..(units or param.controlspec.units or "")
end


-- Raw

--- format_freq_raw
-- @param freq
function Formatters.format_freq_raw(freq)
  if freq < 0.1 then
    freq = util.round(freq, 0.001) .. " Hz"
  elseif freq < 100 then
    freq = util.round(freq, 0.01) .. " Hz"
  elseif util.round(freq, 1) < 1000 then
    freq = util.round(freq, 1) .. " Hz"
  else
    freq = util.round(freq / 1000, 0.01) .. " kHz"
  end
  return freq
end

--- format_secs_raw
-- @param secs
function Formatters.format_secs_raw(secs)
  local out_string
  if secs > 3600 then
    out_string = util.round(secs / 60 / 60, 0.1) .. " h"
  elseif secs >= 120 then
    out_string = util.round(secs / 60, 0.1) .. " min"
  elseif secs >= 60 then
    out_string = util.round(secs) .. " s"
  elseif util.round(secs, 0.01) >= 1 then
    out_string = util.round(secs, 0.1) .. " s"
  else
    out_string = util.round(secs, 0.01)
    if string.len(out_string) < 4 then out_string = out_string .. "0" end
    out_string = out_string .. " s"
  end
  return out_string
end


-- Params

--- default
-- @param param
function Formatters.default(param)
  return Formatters.round(0.01)(param)
end

--- percentage
-- @param param
function Formatters.percentage(param)
  return format(param, util.round(param:get()*100), "%")
end

--- unipolar_as_percentage
-- @param param
function Formatters.unipolar_as_percentage(param)
  return format(param, util.round(param:get()*100), "%")
end

--- bipolar_as_percentage
-- @param param
function Formatters.bipolar_as_percentage(param)
  return format(param, util.round(param:get()*100), "%")
end

--- secs_as_ms
-- @param param
function Formatters.secs_as_ms(param)
  return format(param, util.round(param:get()*1000), "ms")
end

--- unipolar_as_true_false
-- @param param
function Formatters.unipolar_as_true_false(param)
  local str
  if param:get() == 1 then str = "true" else str = "false" end
  return format(param, str)
end

--- unipolar_as_enabled_disabled
-- @param param
function Formatters.unipolar_as_enabled_disabled(param)
  local str
  if param:get() == 1 then str = "enabled" else str = "disabled" end
  return format(param, str)
end

--- bipolar_as_pan_widget
-- @param param
function Formatters.bipolar_as_pan_widget(param)
  local dots_per_side = 10
  local widget
  local function add_dots(num_dots)
    for i=1,num_dots do widget = (widget or "").."." end
  end
  local function add_bar()
    widget = (widget or "").."|"
  end

  local value = param:get()
  local pan_side = math.abs(value)
  local pan_side_percentage = util.round(pan_side*100)
  local descr
  local dots_left
  local dots_right

  if value > 0 then
    dots_left = dots_per_side+util.round(pan_side*dots_per_side)
    dots_right = util.round((1-pan_side)*dots_per_side)
    if pan_side_percentage >= 1 then
      descr = "R"..pan_side_percentage
    end
  elseif value < 0 then
    dots_left = util.round((1-pan_side)*dots_per_side)
    dots_right = dots_per_side+util.round(pan_side*dots_per_side)
    if pan_side_percentage >= 1 then
     descr = "L"..pan_side_percentage
    end
  else
    dots_left = dots_per_side
    dots_right = dots_per_side
  end

  if descr == nil then
    descr = "MID"
  end

  add_bar()
  add_dots(dots_left)
  add_bar()
  add_dots(dots_right)
  add_bar()

  return format(param, descr.." "..widget, "")
end

--- unipolar_as_multimode_filter_freq
-- @param param
function Formatters.unipolar_as_multimode_filter_freq(param)
  local chars = 20
  local widget
  local function add_dots(num) -- TODO: refactor out
    for i=1,num do widget = (widget or "").."." end
  end
  local function add_bars(num) -- TODO: refactor out
    for i=1,num do widget = (widget or "").."|" end
  end

  local value = ControlSpec.BIPOLAR:map(param:get())
  local abs_mapped_value = math.abs(value)
  local percentage = util.round(abs_mapped_value*100)
  local descr

  if value > 0 then
    local clear = value*chars
    if percentage >= 1 then
      descr = "HP"..percentage
    end
    add_bars(1)
    add_dots(clear)
    add_bars(chars-clear+1)
  elseif value < 0 then
    local fill = (value+1)*chars
    if percentage >= 1 then
      descr = "LP"..(100-percentage)
    end
    add_bars(fill+1)
    add_dots(chars-fill)
    add_bars(1)
  else
    add_bars(chars+2)
  end

  if descr == nil then
    descr = "OFF"
  end

  return format(param, widget.." "..descr, "")
end

--- round
-- @param precision
function Formatters.round(precision)
  return function(param)
    return format(param, util.round(param:get(), precision))
  end
end

--- format_freq
-- @param param
function Formatters.format_freq(param)
  return Formatters.format_freq_raw(param:get())
end

--- format_secs
-- @param param
function Formatters.format_secs(param)
  return Formatters.format_secs_raw(param:get())
end

return Formatters
