
local Formatters = {}

function Formatters.unipolar_as_percentage(param)
  return param:string_format(util.round(param:mapped_value()*100), "%")
end

function Formatters.secs_as_ms(param)
  return param:string_format(util.round(param:mapped_value()*1000), "ms")
end

function Formatters.unipolar_as_true_false(param)
  local str
  if param.value == 1 then str = "true" else str = "false" end
  return param:string_format(str)
end

function Formatters.unipolar_as_enabled_disabled(param)
  local str
  if param.value == 1 then str = "enabled" else str = "disabled" end
  return param:string_format(str)
end

function Formatters.bipolar_as_pan_widget(param)
  local dots_per_side = 10
  local widget
  local function add_dots(num_dots)
    for i=1,num_dots do widget = (widget or "").."." end
  end
  local function add_bar()
    widget = (widget or "").."|"
  end

  local mapped_value = param:mapped_value()
  local pan_side_percentage = util.round(math.abs(mapped_value)*100)
  local descr

  if mapped_value > 0 then
    dots_left = dots_per_side+util.round(pan_side_percentage/dots_per_side)
    dots_right = util.round((100-pan_side_percentage)/dots_per_side)
    if pan_side_percentage >= 1 then
      descr = "R"..pan_side_percentage
    end
  elseif mapped_value < 0 then
    dots_left = util.round((100-pan_side_percentage)/dots_per_side)
    dots_right = dots_per_side+util.round(pan_side_percentage/dots_per_side)
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

  return param:string_format(widget.." "..descr, "")
end

function Formatters.unipolar_as_multimode_filter_freq(param)
  local chars = 20
  local widget
  local function add_dots(num) -- TODO: refactor out
    for i=1,num do widget = (widget or "").."." end
  end
  local function add_bars(num) -- TODO: refactor out
    for i=1,num do widget = (widget or "").."|" end
  end

  local mapped_value = ControlSpec.bipolar_spec():map(param.value)
  local abs_mapped_value = math.abs(mapped_value)
  local percentage = util.round(abs_mapped_value*100)
  local descr

  if mapped_value > 0 then
    local clear = mapped_value*chars
    if percentage >= 1 then
      descr = "HP"..percentage
    end
    add_bars(1)
    add_dots(clear)
    add_bars(chars-clear+1)
  elseif mapped_value < 0 then
    local fill = (mapped_value+1)*chars
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

  return param:string_format(widget.." "..descr, "")
end

function Formatters.round(precision)
  return function(param)
    return param:string_format(util.round(param:mapped_value(), precision))
  end
end

return Formatters
