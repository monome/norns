-- @name pusher
-- @version 0.1.0
-- @author jah
-- @txt capture & playback a sound

-- specify dsp engine to load:
engine = 'JustSample'

-- pages
local pageFIRST = 1
local pageRELEASE_TO_PLAY = 2
local pageMASH = 3

local currentPage = pageFIRST

-- param groups
local paramPOS = 1
local paramRATE = 2
local paramFILTER = 3
local paramMIX = 4
local paramDELAY = 5
local paramREVERB = 6

-- warps
local warpLIN = 1
local warpEXP = 2

local currentParam = paramPOS

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

local speed_spec_map = function(value)
	-- -> a ControlSpec(0, 100, 'lin', 0, 1, "")
	-- return linlin(0, 1, 0, 100, value)
	return spec_map(0.125, 8, warpEXP, 0, 1, "", value)
end

local delay_time_spec_map = function(value)
	-- \delay.asSpec;
	-- -> a ControlSpec(0.0001, 1, 'exp', 0, 0.3, " secs")
	-- return linexp(0, 1, 0.0001, 1, value)
	return spec_map(0.0001, 3, warpEXP, 0, 0.3, "secs", value)
end

local decay_time_spec_map = function(value)
	-- \delay.asSpec;
	-- -> a ControlSpec(0.0001, 1, 'exp', 0, 0.3, " secs")
	-- return linexp(0, 1, 0.0001, 1, value)
	return spec_map(0.0001, 3, warpEXP, 0, 0.3, "secs", value)
end

local cutoff_spec_map = function(value)
	-- \midfreq.asSpec;
	-- -> a ControlSpec(25, 4200, 'exp', 0, 440, " Hz")
	-- \freq.asSpec;
	-- -> a ControlSpec(20, 20000, 'exp', 0, 440, " Hz")
	-- return linexp(0, 1, 20, 20000, value)
	return spec_map(20, 10000, warpEXP, 0, 440, "Hz", value)
end

local volume_spec_map = function(value)
	-- return linlin(0, 1, -60, 0, value)
	return spec_map(-60, 0, warpLIN, 0, 0, "dB", value)
end

local display_param = function()
	s.move(0, 16)
    if currentParam == paramPOS then
	    s.text("Start: "..roundi(startPos*100).."%")
	    s.move(0, 24)
	    s.text("End: "..roundi(endPos*100).."%")
    elseif currentParam == paramRATE then
	    s.text("Speed: "..round(speed_spec_map(speed), 2))
	    s.move(0, 24)
	    s.text("") -- TODO: something?
    elseif currentParam == paramFILTER then
	    s.text("Cutoff: "..round(cutoff_spec_map(cutoff), 2).."Hz")
	    s.move(0, 24)
	    s.text("Resonance: "..roundi(resonance*100).."%")
    elseif currentParam == paramMIX then
	    s.text("Delay Level: "..round(volume_spec_map(delayLevel), 2).."dB")
	    s.move(0, 24)
	    s.text("Reverb Level: "..round(volume_spec_map(reverbLevel), 2).."dB")
    elseif currentParam == paramDELAY then
	    s.text("Delay Time: "..round(delay_time_spec_map(delayTime)*1000, 2).."ms")
	    s.move(0, 24)
	    s.text("Decay Time: "..round(decay_time_spec_map(decayTime)*1000, 2).."ms")
    elseif currentParam == paramREVERB then
	    s.text("Reverb Room: "..roundi(reverbRoom*100).."%")
	    s.move(0, 24)
	    s.text("Reverb Damp: "..roundi(reverbDamp*100).."%")
    end
end

local changeToPage = function(page)
    currentPage = page
    redraw()
end

redraw = function()
	s.clear()
    s.level(15)
   	s.move(0, 8)
	if currentPage == pageFIRST then
		s.text("Press and hold rightmost key")
		s.move(0, 16)
		s.text("to capture a sound!")
        --[[
		s.move(0, 48)
		s.text("(Long press leftmost key for")
		s.move(0, 56)
		s.text("help and options)")
        ]]
	elseif currentPage == pageRELEASE_TO_PLAY then
		s.text("Recording...")
		s.move(0, 24)
		s.text("Release key and recorded")
		s.move(0, 32)
		s.text("sound will start playing")
	elseif currentPage == pageMASH then
		s.text("Encoders adjust params")
        display_param()
		s.move(0, 40)
		s.text("Middle key cycles between")
		s.move(0, 48)
		s.text("params, rightmost key will")
		s.move(0, 56)
		s.text("capture a new sound.")
	end

    s.refresh()
end

local set_defaults = function()
    startPos = 0
    endPos = 1.0
    speed = 0.5 -- \rate.asSpec.unmap(\rate.asSpec.default)
    cutoff = 1.0
    resonance = 0
    delayLevel = 0
    reverbLevel = 0
    delayTime = 0.77664218967806
    decayTime = 0.77664218967806
    reverbRoom = 0.5
    reverbDamp = 0.5
end

init = function()
    -- print to command line
    print("pusher!")
    -- add log message
    sys.log.post("hello from pusher!")

    -- screen: turn on anti-alias
    s.aa(1)
    s.line_width(1.0) 
    set_defaults()
    redraw()
end

local add_clip_delta = function(value, delta)
	return math.max(math.min(1, value + delta/100), 0)
end
	
enc = function(n, delta)
	if n == 2 then
        if currentParam == paramPOS then
	    	startPos = add_clip_delta(startPos, delta)
	    	e.startPos(startPos)
        elseif currentParam == paramRATE then
	    	speed = add_clip_delta(speed, delta)
	    	e.speed(speed)
        elseif currentParam == paramFILTER then
	    	cutoff = add_clip_delta(cutoff, delta)
	    	e.cutoff(cutoff)
        elseif currentParam == paramMIX then
	    	delayLevel = add_clip_delta(delayLevel, delta)
	    	e.delayLevel(delayLevel)
        elseif currentParam == paramDELAY then
	    	delayTime = add_clip_delta(delayTime, delta)
	    	e.delayTime(delayTime)
        elseif currentParam == paramREVERB then
	    	reverbRoom = add_clip_delta(reverbRoom, delta)
	    	e.reverbRoom(reverbRoom)
        end
	elseif n == 3 then
        if currentParam == paramPOS then
		    endPos = add_clip_delta(endPos, delta)
		    e.endPos(endPos)
        elseif currentParam == paramRATE then
			print("TODO")
        elseif currentParam == paramFILTER then
	    	resonance = add_clip_delta(resonance, delta)
	    	e.resonance(resonance)
        elseif currentParam == paramMIX then
	    	reverbLevel = add_clip_delta(reverbLevel, delta)
	    	e.reverbLevel(reverbLevel)
        elseif currentParam == paramDELAY then
	    	decayTime = add_clip_delta(decayTime, delta)
	    	e.decayTime(decayTime)
        elseif currentParam == paramREVERB then
	    	reverbDamp = add_clip_delta(reverbDamp, delta)
	    	e.reverbDamp(reverbDamp)
        end
	end
	changeToPage(pageMASH)
end

local handle_key3_release = false

key = function(n, z)
	if n == 2 and z == 1 then
        if currentParam == paramREVERB then
            currentParam = 1
        else
            currentParam = currentParam + 1
        end
        changeToPage(pageMASH)
    elseif n == 3 then
		if z == 1 then
            print("key3 press")
			e.record()
            set_defaults()
			changeToPage(pageRELEASE_TO_PLAY)
		else
            -- if handle_key3_release then
                print("key3 release")
    			e.play()
				changeToPage(pageMASH)
            --[[
            else
                print("select > pusher > up") -- TODO: should not be here
                -- TODO: suggest to fi in norns lua framework
                handle_key3_release = true
            end
            ]]
		end
	end
end
