-- @name knaster
-- @version 0.1.0
-- @author jah
-- @txt a very basic example

-- specify dsp engine to load:
engine = 'Knaster'

-- init function
init = function()
    -- print to command line
    print("knaster!")
    -- add log message
    sys.log.post("hello from knaster!")
    -- set engine params
    e.volume(0)
    -- screen: turn on anti-alias
    s.aa(0)
    s.line_width(1.0) 
end

-- screen redraw function
redraw = function()
    -- clear screen
    s.clear()

    for i=1,6 do
        -- move cursor
        s.move(0,i*8-1)
        -- set pixel brightness (0-15)
        s.level(i)
        -- draw text
        s.text("knaster "..i)
    end 
    s.move(0,7*8-1)
    s.level(15)
    s.text("knaster "..7)

    -- TODO: amp_out-polls not working atm
    --[[
    s.level(15)
    s.move(0,40)
    s.aa(1)
    s.line(vu,40)
    s.stroke()
    ]]

    s.update()
end 

-- TODO: amp_out-polls not working atm
--[[
p = nil
vu = 0

local function calcMeter(amp, n, floor)
   n = n or 64
   floor = floor or -72
   local db = 20.0 * math.log10(amp)
   local norm = 1.0 - (db / floor)
   vu = norm * n
   redraw()
end

local ampCallback = function(amp)
    print(amp)
    calcMeter(amp, 64, -72)
end

poll.report = function(polls)
   p = polls['amp_out_l']
   if p then
      p.callback = ampCallback
      p.time = 0.03;
      p:start()
   else
      print("couldn't get requested poll, dang")
   end 
end 


cleanup = function()
   if p then p:stop() end
end 
]]
