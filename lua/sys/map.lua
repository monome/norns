--- map.lua
local map = {}

local mNormal = 0
local mMap = 1
local mAlt = 2 

map.state = mNormal

local pending = false
local metro = require 'metro'
local t = metro[31]
t.count = 2
t.callback = function(stage)
    if(stage==2) then
        map.set_mode(mAlt)
        pending = false
    end
end



-- default handlers, to be overwritten by app

redraw = function()
    s.clear()
end

local nowhere = {}

nowhere.key = function(n,z)
    print("key "..n.." > "..z)
end

nowhere.enc = function(n, delta)
	print("enc "..n.." > "..delta)
end

key = nowhere.key
enc = nowhere.enc

nowhere.s = function() end

s = {}
restore_s = function()
    s.clear = s_clear
    s.level = s_level
    s.line = s_line
    s.move = s_move
    s.text = s_text
end
block_s = function()
    s.clear = nowhere.s
    s.level = nowhere.s
    s.line = nowhere.s
    s.move = nowhere.s
    s.text = nowhere.s
end


-- input redirection
norns.enc = function(n, delta)
    if(n==1) then
        print("map enc")
    else 
        if(map.state==mNormal) then
            enc(n, delta)
        else
            print("map.state")
        end
    end
end

norns.key = function(n, z)
    -- map key mode detection
	if(n==1) then
		if z==1 and map.state==mMap then
            map.set_mode(mNormal)  
        elseif z==1 and map.state==mNormal then
            pending = true
            t.time = 0.5
            t:start()
        elseif z==0 and map.state==mAlt then
            map.set_mode(mNormal)
        elseif z==0 and pending==true then
            map.set_mode(mMap)
            t:stop()
            pending = false
		end
    -- key 2/3 conditional pass
	else 
		if map.state==mNormal then
			key(n,z)
		else
			print("map.state")
		end 
	end
end


map.set_mode = function(mode)
    if mode==mNormal then
        map.state = mNormal

        restore_s()
        redraw()

    elseif mode==mMap then
        map.state = mMap
        block_s()

        s_clear()
        s_level(15)
	    list = scandir(script_dir)
	    for i=1,5 do
		    s_move(0,10*i)
            line = string.gsub(list[i],'.lua','')
            s_text(line) 
	    end


    elseif mode==mAlt then
        map.state = mAlt
        block_s()

        s_clear()
        s_move(20,20)
        s_level(15)
        s_text("alt")
    end
end


scandir = function(directory)
    local i, t, popen = 0, {}, io.popen
    local pfile = popen('ls "'..directory..'"')
    for filename in pfile:lines() do
        i = i + 1
        t[i] = filename
    end
    pfile:close()
    return t
end


-- startup
map.set_mode(mNormal)
