-- screen redirection functions
s = {}
sys.s = {}

sys.s.restore = function()
    s.update = s_update
    s.aa = s_aa
    s.clear = s_clear
    s.level = s_level
    s.line = s_line
    s.arc = s_arc
    s.line_width = s_line_width
    s.move = s_move
    s.stroke = s_stroke
    s.fill = s_fill
    s.text = s_text
    s.text_right = s_text_right
    s.text_center = s_text_center
    s.close = s_close
    s.font_face = s_font_face
    s.font_size = s_font_size
    s.circle = s_circle
    s.rect = s_rect
    s.curve = s_curve
    s.curve_rel = s_curve_rel
end

sys.s.block = function()
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
