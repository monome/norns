-- drawing

-- specify dsp engine to load:
engine = 'TestSine'

-- init function
init = function(commands, count)
    redraw()
end

-- screen redraw function
redraw = function()
    -- clear screen
    s.clear()
    -- set pixel brightness (0-15)
    s.level(15)
    -- enable anti-alasing
    s.aa(1)
    -- set line width
    s.line_width(1.0) 
    -- move position
    s.move(0,0)
    -- draw line
    s.line(10,20)
    -- stroke line
    s.stroke()
    -- draw arc: x-center, y-center, radius, angle1, angle2
    s.arc(20,0,10,0,math.pi*0.8)
    s.stroke()
    -- draw rect: x,y,w,h
    s.rect(30,10,15,20)
    s.level(3)
    s.stroke()
    -- draw curve
    s.move(50,0)
    s.curve(50,20,60,0,70,10)
    s.level(15)
    s.stroke()
    -- draw poly and fill
    s.move(60,20)
    s.line(80,10)
    s.line(70,40)
    s.close()
    s.level(10)
    s.fill()
    -- draw circle
    s.circle(100,20,10)
    s.stroke()
    

    s.level(15)
    s.move(0,63)
    -- set text face
    s.font_face(9)
    -- set text size
    s.font_size(20)
    -- draw text
    s.text("new!") 
    -- draw centered text
    s.move(63,50)
    s.font_face(0)
    s.font_size(8)
    s.text_center("center")
    -- draw right aliged text
    s.move(127,63)
    s.text_right("1992")
end
