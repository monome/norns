--- map.lua
local map = {}

local mNormal = 0
local mNav = 1
local mAlt = 2 

map.state = mNormal

local pending = false
local metro = require 'metro'
local t = metro[31]
t.count = 2
t.callback = function(stage)
    if(stage==2) then
        map.set(mAlt)
        pending = false
    end
end



init = function() 
    map.set(map.state)
end

norns.blank = function() s.clear() end

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
    s.aa = norns.none
    s.clear = norns.none
    s.level = norns.none
    s.line = norns.none
    s.move = norns.none
    s.stroke = norns.none
    s.text = norns.none
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
		if z==1 and map.state==mNav then
            map.set(mNormal)  
        elseif z==1 and map.state==mNormal then
            pending = true
            t.time = 0.5
            t:start()
        elseif z==0 and map.state==mAlt then
            map.set(mNormal)
        elseif z==0 and pending==true then
            map.set(mNav)
            t:stop()
            pending = false
		end
    -- key 2/3 conditional pass
	else 
        map.key(n,z)
	end
end


map.set = function(mode)
    if mode==mNormal then
        map.state = mNormal 
        restore_s()
        map.key =  key
        map.enc = enc
        map.level = map.nav.level
        redraw()

    elseif mode==mNav then
        map.state = mNav
        block_s()
        map.key = map.nav.key
        map.enc = map.nav.enc
        map.level = map.nav.level
        map.nav.redraw()

    elseif mode==mAlt then
        map.state = mAlt
        block_s()
        map.key = map.alt.key
        map.enc = map.alt.enc
        map.level = map.alt.level
        map.alt.redraw()
    end
end

----------------------------------------------------
-- interfaces

----------
-- nav
map.nav = {}
map.nav.pos = 0
map.nav.offset = 0
map.nav.list = scandir(script_dir)
map.nav.len = tablelength(map.nav.list)
map.nav.out = 0

map.nav.key = function(n,z)
    if n==2 and z==1 then 
        line = string.gsub(map.nav.list[map.nav.pos+1],'.lua','')
        norns.script.load(line)
        map.set(mNormal)
    end
end

map.nav.enc = function(n,delta)
    if n==2 then
        map.nav.pos = map.nav.pos + delta
		if map.nav.pos > map.nav.len - 1 then map.nav.pos = map.nav.len - 1 end
		if map.nav.pos < 0 then map.nav.pos = 0 end
		if map.nav.pos > 2 or map.nav.offset > 0 then
			map.nav.offset = map.nav.offset + delta
			if map.nav.offset < 0 then map.nav.offset = 0 end
			if map.nav.offset > map.nav.len - 3 then map.nav.offset = map.nav.len - 3 end
		end
        map.nav.redraw()
    end
end

map.nav.level = function(delta)
    map.nav.out = map.nav.out + delta
    if map.nav.out < 0 then map.nav.out = 0 
    elseif map.nav.out > 63 then map.nav.out = 63 end
    --level_out(map.nav.out,0) 
    --level_out(map.nav.out,1) 
    level_hp(map.nav.out)
end 

map.nav.redraw = function()
    s_clear()
    s_level(15)
    for i=1,6 do
		if i < map.nav.len - map.nav.offset + 1 then
        	s_move(0,10*i)
        	line = string.gsub(map.nav.list[i+map.nav.offset],'.lua','')
        	if(i==map.nav.pos-map.nav.offset+1) then
            	s_level(15)
        	else
            	s_level(4)
        	end
        	s_text(string.upper(line)) 
		end
     end
end

----------
-- alt
map.alt = {}
map.alt.key = function(n,z)
end

map.alt.enc = function(n,delta)
end

map.alt.level = function(delta)
end

map.alt.redraw = function()
    s_clear()
    s_aa(1)
    s_level(2)
    s_move(0,1)
    s_line(100,1)
    s_stroke()

    s_level(10)
    s_move(0,0)
    s_line(norns.batterypercent,0)
    s_stroke()

    if norns.powerpresent==1 then
        s_move(104,0)
        s_line(106,0)
        s_move(104,1)
        s_line(106,1)
        s_stroke()
    end

    s_move(0,50)
    s_level(15)
    s_text(norns.state.script)
end

