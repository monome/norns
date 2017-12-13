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


init = function() end

connect = function() 
    map.set_mode(map.state)
    init()
end

-- default handlers, to be overwritten by app

redraw = function()
    s.clear()
end

local nowhere = function() end

key = nowhere
enc = nowhere

s = {}
local restore_s = function()
    s.aa = s_aa
    s.clear = s_clear
    s.level = s_level
    s.line = s_line
    s.move = s_move
    s.stroke = s_stroke
    s.text = s_text
end
local block_s = function()
    s.aa = nowhere
    s.clear = nowhere
    s.level = nowhere
    s.line = nowhere
    s.move = nowhere
    s.stroke = nowhere
    s.text = nowhere
end

-- input redirection
norns.enc = function(n, delta)
    if(n==1) then
        map.level(delta)
    else 
        if(map.state==mNormal) then
            enc(n, delta)
        else
            map.enc(n, delta)
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
        map.key(n,z)
	end
end


map.set_mode = function(mode)
    if mode==mNormal then
        map.state = mNormal 
        restore_s()
        map.key =  key
        map.enc = enc
        map.level = map.level_map
        redraw()

    elseif mode==mMap then
        map.state = mMap
        block_s()
        map.key = map.key_map
        map.enc = map.enc_map
        map.level = map.level_map
        map.nav.refresh()

    elseif mode==mAlt then
        map.state = mAlt
        block_s()
        map.key = map.key_alt
        map.enc = map.enc_alt
        map.level = map.level_alt

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

-- interfaces
map.nav = {}
map.key_map = function(n,z)
    print("key>map")
end

map.nav.refresh = function()
    s_clear()
    s_level(15)
    list = scandir(script_dir)
    for i=1,6 do
        s_move(0,10*i)
        line = string.gsub(list[i],'.lua','')
        s_text(line) 
     end
end

map.key_alt = function(n,z)
    print("key>alt")
end

map.enc_map = function(n,delta)
end

map.enc_alt = function(n,delta)
end

map.level_map = function(delta)
    print('lvl')
end

map.level_alt = function(delta)
    print('alt-lvl')
end
