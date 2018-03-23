--- Screen class
-- @module screen
-- @alias Screen
local Screen = {} 

--- copy buffer to screen 
Screen.update = function() s_update end

--- enable/disable anti-aliasing
-- @param state on(1) or off(0)
Screen.aa = function(state) s_aa(state) end 

--- clear
Screen.clear = function() s_clear end

--- set level (color/brightness)
-- @param value 0-15 (0=off, 15=white)
Screen.level = function(value) s_level(value) end

--- draw line to specified point
-- @param x destination x
-- @param y destination y 
Screen.line = function() s_line() end

Screen.arc = s_arc
Screen.line_width = s_line_width
Screen.move = s_move
Screen.stroke = s_stroke
Screen.fill = s_fill
Screen.text = s_text
Screen.text_right = s_text_right
Screen.text_center = s_text_center
Screen.close = s_close
Screen.font_face = s_font_face
Screen.font_size = s_font_size
Screen.circle = s_circle
Screen.rect = s_rect
Screen.curve = s_curve
Screen.curve_rel = s_curve_rel

--- enable screen functions (allow user script to modify screen)
s_enable = function()
    Screen.update = s_update
    Screen.aa = s_aa
    Screen.clear = s_clear
    Screen.level = s_level
    Screen.line = s_line
    Screen.arc = s_arc
    Screen.line_width = s_line_width
    Screen.move = s_move
    Screen.stroke = s_stroke
    Screen.fill = s_fill
    Screen.text = s_text
    Screen.text_right = s_text_right
    Screen.text_center = s_text_center
    Screen.close = s_close
    Screen.font_face = s_font_face
    Screen.font_size = s_font_size
    Screen.circle = s_circle
    Screen.rect = s_rect
    Screen.curve = s_curve
    Screen.curve_rel = s_curve_rel
end

--- disable screen functions (menu only controls screen)
s_disable = function()
    for k,v in pairs(s) do
	s[k] = sys.none
    end
end

s_text_right = function(str)
    x,y = s_extents(str)
    s_move_rel(-x,0)
    s_text(str)
end 

s_text_center = function(str)
    x,y = s_extents(str)
    s_move_rel(-x/2,0)
    s_text(str)
end 

s_circle = function(x,y,r)
    s_arc(x,y,r,0,math.pi*2)
end

return Screen
