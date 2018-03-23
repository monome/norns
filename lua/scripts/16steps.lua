-- @name 16steps
-- @version 0.1.0
-- @author jah
-- @txt 16x8 step sequencer

engine = 'Step'

TRIG_LEVEL = 15
PLAYPOS_LEVEL = 7
CLEAR_LEVEL = 0

-- warps
local warpLIN = 1
local warpEXP = 2

local linexp = function(slo, shi, dlo, dhi, f)
    if f < slo then
        return dlo
    elseif f >= shi then
        return dhi
    else
        return math.pow( dhi/dlo, (f-slo) / (shi-slo) ) * dlo
    end
end

local linlin = function(slo, shi, dlo, dhi, f)
    if f < slo then
        return dlo
    elseif f >= shi then
        return dhi
    else
        return (f-slo) / (shi-slo) * (dhi-dlo) + dlo
    end
end

local roundi = function(number)
    return math.floor(number+0.5) -- TODO: should equal round(number, 0)
end

local round = function(number, precision)
	local a = precision * 10
    return math.floor(number*a+0.5)/a
end

local spec_map = function(minval, maxval, warp, step, default, units, value)
	if warp == warpEXP then
		-- TODO: step only applicable to warpLIN?
		return linexp(0, 1, minval, maxval, value)
	else -- only warpEXP and warpLIN available
		return linlin(0, 1, minval, maxval, value)
		-- TODO: constrain to step
	end
end

 
local tempo_spec_map = function(value)
	return spec_map(20, 300, warpLIN, 0, 120, "BPM", value)
end

local swing_amount_spec_map = function(value)
	return spec_map(0, 100, warpLIN, 0, 0, "%", value)
end

local refresh_trig_on_grid = function(x, y) -- TODO: naming, this is not really refresh but intermediate update
    if g then
    	if trigs[y*width+x] then
    		g:led(x, y, TRIG_LEVEL)
    	elseif x-1 == playpos then
    		g:led(x, y, PLAYPOS_LEVEL)
    	else
    		g:led(x, y, CLEAR_LEVEL)
    	end
        g:refresh()
    end
end

local refresh_trig_col_on_grid = function(x) -- TODO: naming, this is not really refresh but intermediate update
	for y=1,height do
		refresh_trig_on_grid(x, y)
	end
end

width = 16
height = 8
playpos = nil
swing_amount = 0
tempo = 0.35714285714286 -- TODO: 120
playing = false

trigs = {}
for i=1,width*height do
	trigs[i] = false
end

local trig_is_unset = function(x, y)
    return trigs[y*width+x] == false
end

local trig_is_set = function(x, y)
    return trigs[y*width+x]
end


-- init function
init = function()
    -- print to command line
    print("step!")
    -- add log message
    sys.log.post("hello from step!")
    -- set engine params
    e.setNumSteps(width)
    e.setTempo(tempo_spec_map(tempo))
    e.setSwingAmount(swing_amount_spec_map(swing_amount))
    e.stopSequencer()
    e.clearAllTrigs()
    -- clear grid, if it exists
    if g then 
        g:all(0)
        g:refresh()
    end 
	start_playpos_poll()
end

-- encoder function
enc = function(n, delta)
    if n == 2 then
        tempo = clamp(tempo+delta/200, 0, 1)
        e.setTempo(tempo_spec_map(tempo))
    elseif n == 3 then
        swing_amount = clamp(swing_amount+delta/200, 0, 1)
        e.setSwingAmount(swing_amount_spec_map(swing_amount))
    end
    redraw()
end

-- key function
key = function(n, z)
    if n == 2 and z == 1 then
        -- e.clearPattern()
		e.stopSequencer()
        playing = false
    elseif n == 3 and z == 1 then
        -- e.scrambleSamples()
		e.playSequencer()
        playing = true
    end
    redraw()
end

-- screen redraw function
redraw = function()
    -- clear screen
    s.clear()
    -- set pixel brightness (0-15)
    s.level(15)

    -- show timer
    s.move(0,8)
    s.text("16STEPS")

	s.move(0, 24)
    s.text("Tempo: "..round(tempo_spec_map(tempo), 1).."BPM")
    s.move(0, 32)
    s.text("Swing: "..roundi(swing_amount_spec_map(swing_amount)).."%")
    s.move(0, 48)
    if playing then
        s.text("Playing")
    else
        s.text("Stopped")
    end

    s.update()
end

-- grid key function
gridkey = function(x, y, state)
    if state > 0 then 
        -- turn on led
		if trig_is_set(x, y) then
        	e.clearTrig(y-1, x-1)
			trigs[y*width+x] = false
			refresh_trig_on_grid(x, y)
		else
        	e.setTrig(y-1, x-1)
			trigs[y*width+x] = true
			refresh_trig_on_grid(x, y)
		end
        if g then
		    g:refresh()
        end
    end
end 

local playposCallback = function(new_playpos)
        --[[
    if playpos then
        print("playpos "..playpos)
    else
        print("playpos: nil")
    end
    if new_playpos then
        print("new_playpos "..new_playpos)
    else
        print("new_playpos: nil")
    end
    ]]
	if playpos ~= new_playpos then
        local previous_playpos = playpos
        playpos = new_playpos
        if previous_playpos then
		    refresh_trig_col_on_grid(previous_playpos+1)
        end
        if playpos then
		    refresh_trig_col_on_grid(playpos+1)
        end
        if g then
		    g:refresh()
        end
	end
end

start_playpos_poll = function()
   print('starting playpos poll in step')
   p = poll.set('playpos', playposCallback)
   p.time = 0.02;
   p:start()
end 

-- called on script quit, release memory
cleanup = function()
    if g then 
        g:all(0)
        g:refresh()
    end
end
