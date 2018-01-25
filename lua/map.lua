-- map.lua
-- norns screen-based navigation module

local map = {}
norns.map = {}

-- level enums
local pHOME = 1
local pSELECT = 2
local pPREVIEW = 3
local pSTATUS = 4
local pPARAMS = 5
local pSETTINGS = 6
local pWIFI = 7
local pSLEEP = 8

local p = {}
p.key = {}
p.enc = {}
p.redraw = {}
p.init = {}

map.mode = false
map.page = pSELECT

local pending = false
-- metro for key hold detection
local metro = require 'metro'
local t = metro[31]
t.count = 2
t.callback = function(stage)
    if(stage==2) then
        if map.mode == false then
            map.key(1,1)
        end 
        pending = false
    end
end


-- assigns key/enc/screen handlers after user script has loaded
norns.map.init = function() 
    map.set_mode(map.mode)
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
local _enc = {{},{},{}}
_enc[1].sens = 1
_enc[1].tick = 0
_enc[2].sens = 1
_enc[2].tick = 0 
_enc[3].sens = 1
_enc[3].tick = 0

norns.enc = function(n, delta) 
    _enc[n].tick = _enc[n].tick + delta
    if math.abs(_enc[n].tick) > _enc[n].sens then
        _enc[n].tick = 0
        if(map.mode==false) then
            enc(n, delta)
        else
            map.enc(n, delta)
        end
    end
end

set_enc_sens = function(n, sens)
    _enc[n].sens = sens
    _enc[n].tick = 0
end


norns.key = function(n, z)
    -- key 1 detect for short press
	if(n==1) then
        if z==1 then
            pending = true
            t.time = 0.25
            t:start()
        elseif z==0 and pending==true then
            if map.mode == true then map.set_mode(false)
            else map.set_mode(true) end
            t:stop()
            pending = false
        elseif z==0 and map.mode==false then
            map.key(n,z) -- always 1,0
        else
            map.set_mode(false)
 		end
    -- key 2/3 pass
	else 
        map.key(n,z)
	end
end

-- map set mode
map.set_mode = function(mode)
    if mode==false then
        map.mode = false 
        restore_s()
        map.key = key
        map.enc = enc
        set_enc_sens(1,1)
        set_enc_sens(2,1)
        set_enc_sens(3,1)
        redraw() 
    else -- enable map mode
        map.mode = true
        block_s()
        map.set_page(map.page)
        set_enc_sens(1,1)
        set_enc_sens(2,3)
        set_enc_sens(3,16) 
        map.redraw() 
    end
end

-- set page
map.set_page = function(page)
    map.key = p.key[page]
    map.enc = p.enc[page]
    map.redraw = p.redraw[page]
    p.init[page]()
end



-- --------------------------------------------------
-- interfaces

-- HOME

p.home = {}
p.home.pos = 0
p.home.list = {"SELECT >", "PARAMETERS >", "SETTINGS >", "SLEEP >"}
p.home.len = 4

p.init[pHOME] = norns.none

p.key[pHOME] = function(n,z)
    if n==2 and z==1 then
        print("STATUS MODE") 
    elseif n==3 and z==1 then 
        option = {pSELECT, pPARAM, pSETTINGS, pSLEEP}
        map.set_page(option[p.home.pos+1]) 
        map.redraw()
    end
end 

p.enc[pHOME] = function(n,delta)
    -- scroll file list
    if n==2 then 
        p.home.pos = p.home.pos + delta 
	    if p.home.pos > p.home.len - 1 then p.home.pos = p.home.len - 1
        elseif p.home.pos < 0 then p.home.pos = 0 end
        map.redraw()
    end
end

p.redraw[pHOME] = function()
    -- draw file list and selector
    s_clear()
    s_level(10)
    s_move(0,10)
    s_text("norns v"..norns.version.norns)
    for i=3,6 do
       	s_move(0,10*i)
       	line = string.gsub(p.home.list[i-2],'.lua','')
       	if(i==p.home.pos+3) then
           	s_level(15)
       	else
           	s_level(4)
       	end
       	s_text(string.upper(line)) 
     end
end




-- SELECT

p.sel = {}
p.sel.pos = 0
p.sel.list = scandir(script_dir)
p.sel.len = tablelength(p.sel.list)
p.sel.depth = 0
p.sel.folders = {}

p.sel.dir = function()
    local path = script_dir
    for k,v in pairs(p.sel.folders) do
        path = path .. v
    end
    print("path: "..path)
    return path
end

p.init[pSELECT] = function()
    p.sel.list = scandir(script_dir)
    p.sel.len = tablelength(p.sel.list)
end

p.key[pSELECT] = function(n,z)
    -- back
    if n==2 and z==1 then
        if p.sel.depth > 0 then
            print('back')
            p.sel.folders[p.sel.depth] = nil
            p.sel.depth = p.sel.depth - 1
            -- FIXME return to folder position
            p.sel.list = scandir(p.sel.dir())
            p.sel.len = tablelength(p.sel.list)
            p.sel.pos = 0
            map.redraw()
        else
            map.set_page(pHOME)
            map.redraw()
        end 
    -- select
    elseif n==3 and z==1 then 
        local s = p.sel.list[p.sel.pos+1]
        if string.find(s,'/') then 
            print("folder")
            p.sel.depth = p.sel.depth + 1
            p.sel.folders[p.sel.depth] = s
            p.sel.list = scandir(p.sel.dir())
            p.sel.len = tablelength(p.sel.list)
            p.sel.pos = 0
            map.redraw()
        else 
            --line = string.gsub(s,'.lua','')
            local path = ""
            for k,v in pairs(p.sel.folders) do
                path = path .. v
            end
            path = path .. s
            norns.script.load(path)
            map.set_mode(false)
        end
    end
end

p.enc[pSELECT] = function(n,delta)
    -- scroll file list
    if n == 1 then
        p.sel.level(delta)
    elseif n==2 then 
        p.sel.pos = p.sel.pos + delta 
	    if p.sel.pos > p.sel.len - 1 then p.sel.pos = p.sel.len - 1
        elseif p.sel.pos < 0 then p.sel.pos = 0 end
        map.redraw()
    elseif n==3 then
        p.sel.page = 1 - p.sel.page
        print("page "..p.sel.page)
    end
end


p.redraw[pSELECT] = function()
    -- draw file list and selector
    s_clear()
    s_level(15)
    for i=1,6 do
		if (i > 2 - p.sel.pos) and (i < p.sel.len - p.sel.pos + 3) then
        	s_move(0,10*i)
        	line = string.gsub(p.sel.list[i+p.sel.pos-2],'.lua','')
        	if(i==3) then
            	s_level(15)
        	else
            	s_level(4)
        	end
        	s_text(string.upper(line)) 
		end
     end
end

map.alt = {}
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

map.level = function(delta)
    norns.state.out = norns.state.out + delta
    if norns.state.out < 0 then norns.state.out = 0 
    elseif norns.state.out > 64 then norns.state.out = 64 end
    print("level: " .. norns.state.out)
    --level_out(norns.state.out,0) 
    --level_out(norns.state.out,1) 
    --level_hp(norns.state.out)
    --level_out(norns.state.out/64)
end 


