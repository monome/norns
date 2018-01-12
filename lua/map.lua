-- map.lua
-- norns screen-based navigation module
-- also contains 'alt' view, by holding map key

local map = {}
norns.map = {}

-- mode enums
local mRun = 0
local mNav = 1
local mAlt = 2 

map.state = mRun

local pending = false
local previous = mRun
-- metro for key hold detection
local metro = require 'metro'
local t = metro[31]
t.count = 2
t.callback = function(stage)
    if(stage==2) then
        previous = map.state
        map.set(mAlt)
        pending = false
    end
end


-- assigns key/enc/screen handlers after user script has loaded
norns.map.init = function() 
    map.set(map.state)
end

-- redirection for scripts that don't define refresh()
norns.blank = function() s.clear() end

-- screen redirection functions
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
local _enc = {{},{}}
_enc[1].sens = 1
_enc[1].tick = 0
_enc[2].sens = 1
_enc[2].tick = 0

norns.enc = function(n, delta)
    -- level enc always managed by map script
    if(n==1) then
        map.level(delta)
    -- other encs conditionally passed
    elseif(n==2) then
        _enc[1].tick = _enc[1].tick + delta
        if math.abs(_enc[1].tick) > _enc[1].sens then
            _enc[1].tick = 0
            if(map.state==mRun) then
                -- offset for user script
                enc(1, delta)
            else
                map.enc(1, delta)
            end
        end
    elseif(n==3) then
        _enc[2].tick = _enc[2].tick + delta
        if math.abs(_enc[2].tick) >= _enc[2].sens then
            _enc[2].tick = 0
            if(map.state==mRun) then
                -- offset for user script
                enc(2, delta)
            else
                map.enc(2, delta)
            end
        end
    end
end

set_enc_sens = function(n, sens)
    _enc[n].sens = sens
    _enc[n].tick = 0
end


norns.key = function(n, z)
    -- map key mode detection
	if(n==1) then
        if z==1 then
            pending = true
            t.time = 0.5
            t:start()
        elseif z==0 and map.state==mAlt then
            map.set(previous)
        elseif z==0 and pending==true then
            if map.state == mNav then map.set(mRun)
            else map.set(mNav) end
            t:stop()
            pending = false
		end
    -- key 2/3 conditional pass
	else 
        -- remap to key 1/2
        map.key(n-1,z)
	end
end

-- map set mode
map.set = function(mode)
    if mode==mRun then
        map.state = mRun 
        restore_s()
        map.key =  key
        map.enc = enc
        map.level = map.nav.level
        set_enc_sens(1,1)
        set_enc_sens(2,1)
        redraw() 
    elseif mode==mNav then
        map.state = mNav
        block_s()
        map.key = map.nav.key
        map.enc = map.nav.enc
        map.level = map.nav.level
        set_enc_sens(1,3)
        set_enc_sens(2,16)
        map.nav.list = scandir(map.nav.dir())
        map.nav.len = tablelength(map.nav.list)
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


-- --------------------------------------------------
-- interfaces

-- --------
-- nav
map.nav = {}
map.nav.pos = 0
map.nav.list = scandir(script_dir)
map.nav.len = tablelength(map.nav.list)
map.nav.depth = 0
map.nav.folders = {}
map.nav.page = 0

map.nav.dir = function()
    local path = script_dir
    for k,v in pairs(map.nav.folders) do
        path = path .. v
    end
    print("path: "..path)
    return path
end

map.nav.key = function(n,z)
    -- back
    if n==1 and z==1 then
        if map.nav.depth > 0 then
            print('back')
            map.nav.folders[map.nav.depth] = nil
            map.nav.depth = map.nav.depth - 1
            -- FIXME return to folder position
            map.nav.list = scandir(map.nav.dir())
            map.nav.len = tablelength(map.nav.list)
            map.nav.pos = 0
            map.nav.redraw()
        end 
    -- select
    elseif n==2 and z==1 then 
        local s = map.nav.list[map.nav.pos+1]
        if string.find(s,'/') then 
            print("folder")
            map.nav.depth = map.nav.depth + 1
            map.nav.folders[map.nav.depth] = s
            map.nav.list = scandir(map.nav.dir())
            map.nav.len = tablelength(map.nav.list)
            map.nav.pos = 0
            map.nav.redraw()
        else 
            --line = string.gsub(s,'.lua','')
            local path = ""
            for k,v in pairs(map.nav.folders) do
                path = path .. v
            end
            path = path .. s
            norns.script.load(path)
            map.set(mRun)
        end
    end
end

map.nav.enc = function(n,delta)
    -- scroll file list
    if n==1 then 
        map.nav.pos = map.nav.pos + delta 
	    if map.nav.pos > map.nav.len - 1 then map.nav.pos = map.nav.len - 1
        elseif map.nav.pos < 0 then map.nav.pos = 0 end
        map.nav.redraw()
    elseif n==2 then
        map.nav.page = 1 - map.nav.page
        print("page "..map.nav.page)
    end
end

map.nav.level = function(delta)
    norns.state.out = norns.state.out + delta
    if norns.state.out < 0 then norns.state.out = 0 
    elseif norns.state.out > 64 then norns.state.out = 64 end
    --level_out(norns.state.out,0) 
    --level_out(norns.state.out,1) 
    --level_hp(norns.state.out)
    level_out(norns.state.out/64)
end 

map.nav.redraw = function()
    -- draw file list and selector
    s_clear()
    s_level(15)
    for i=1,6 do
		if (i > 2 - map.nav.pos) and (i < map.nav.len - map.nav.pos + 3) then
        	s_move(0,10*i)
        	line = string.gsub(map.nav.list[i+map.nav.pos-2],'.lua','')
        	if(i==3) then
            	s_level(15)
        	else
            	s_level(4)
        	end
        	s_text(string.upper(line)) 
		end
     end
end

-- --------
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

    status = "battery > "..norns.batterypercent 
    if norns.powerpresent==1 then status = status.." (powered)" end

    -- draw battery bar
    s_level(10)
    s_move(0,10)
    s_text(status)

    s_move(0,20)
    local net = 'ip > '..os.capture("ifconfig wlan0| grep 'inet ' | awk '{print $2}'")
    if net == 'ip > ' then net = 'no wifi' end
    s_text(net)

    -- draw current script loaded
    s_move(0,60)
    s_level(15)
    s_text("script > "..norns.state.script)
end

