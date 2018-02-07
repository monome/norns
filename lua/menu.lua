-- menu.lua
-- norns screen-based navigation module

local menu = {}

-- level enums
local pHOME = 1
local pSELECT = 2
local pPREVIEW = 3
local pSTATUS = 4
local pPARAMS = 5
local pSETTINGS = 6
local pWIFI = 7
local pSLEEP = 8
local pLOG = 9

local p = {}
p.key = {}
p.enc = {}
p.redraw = {}
p.init = {}

menu.mode = false
menu.page = pHOME

local pending = false
-- metro for key hold detection
local metro = require 'metro'
local t = metro[31]
t.count = 2
t.callback = function(stage)
    if(stage==2) then
        if menu.mode == false then
            menu.key(1,1)
        end 
        pending = false
    end
end


-- assigns key/enc/screen handlers after user script has loaded
sys.menu = {}
sys.menu.init = function() 
    menu.set_mode(menu.mode)
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
        if(menu.mode==false) then
            enc(n, delta)
        else
            menu.enc(n, delta)
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
            if menu.mode == true then menu.set_mode(false)
            else menu.set_mode(true) end
            t:stop()
            pending = false
        elseif z==0 and menu.mode==false then
            menu.key(n,z) -- always 1,0
        else
            menu.set_mode(false)
 		end
    -- key 2/3 pass
	else 
        menu.key(n,z)
	end
end

-- menu set mode
menu.set_mode = function(mode)
    if mode==false then
        menu.mode = false 
        sys.s.restore()
        menu.key = key
        menu.enc = enc
        set_enc_sens(1,1)
        set_enc_sens(2,1)
        set_enc_sens(3,1)
        redraw() 
    else -- enable menu mode
        menu.mode = true
        sys.s.block()
        menu.set_page(menu.page)
        set_enc_sens(1,1)
        set_enc_sens(2,4)
        set_enc_sens(3,4) 
    end
end

-- set page
menu.set_page = function(page)
    menu.page = page
    menu.key = p.key[page]
    menu.enc = p.enc[page]
    menu.redraw = p.redraw[page]
    p.init[page]()
    s_font_face(0)
    s_font_size(8)
    menu.redraw()
end



-- --------------------------------------------------
-- interfaces

-- HOME

p.home = {}
p.home.pos = 0
p.home.list = {"SELECT >", "PARAMETERS >", "SETTINGS >", "SLEEP >"}
p.home.len = 4

p.init[pHOME] = sys.none

p.key[pHOME] = function(n,z)
    if n==2 and z==1 then
        menu.set_page(pSTATUS)
    elseif n==3 and z==1 then 
        option = {pSELECT, pPARAMS, pSETTINGS, pSLEEP}
        menu.set_page(option[p.home.pos+1]) 
    end
end 

p.enc[pHOME] = function(n,delta)
    if n==2 then 
        p.home.pos = p.home.pos + delta 
	    if p.home.pos > p.home.len - 1 then p.home.pos = p.home.len - 1
        elseif p.home.pos < 0 then p.home.pos = 0 end
        menu.redraw()
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
p.sel.list = sys.file.scandir(script_dir)
p.sel.len = sys.file.tablelength(p.sel.list)
p.sel.depth = 1
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
    if p.sel.depth == 0 then
        p.sel.list = sys.file.scandir(script_dir)
    else
        p.sel.list = sys.file.scandir(p.sel.dir())
    end
    p.sel.len = sys.file.tablelength(p.sel.list)
end

p.key[pSELECT] = function(n,z)
    -- back
    if n==2 and z==1 then
        if p.sel.depth > 0 then
            print('back')
            p.sel.folders[p.sel.depth] = nil
            p.sel.depth = p.sel.depth - 1
            -- FIXME return to folder position
            p.sel.list = sys.file.scandir(p.sel.dir())
            p.sel.len = sys.file.tablelength(p.sel.list)
            p.sel.pos = 0
            menu.redraw()
        else
            menu.set_page(pHOME)
        end 
    -- select
    elseif n==3 and z==1 then 
        local s = p.sel.list[p.sel.pos+1]
        if string.find(s,'/') then 
            print("folder")
            p.sel.depth = p.sel.depth + 1
            p.sel.folders[p.sel.depth] = s
            p.sel.list = sys.file.scandir(p.sel.dir())
            p.sel.len = sys.file.tablelength(p.sel.list)
            p.sel.pos = 0
            menu.redraw()
        else 
            --line = string.gsub(s,'.lua','')
            local path = ""
            for k,v in pairs(p.sel.folders) do
                path = path .. v
            end
            path = path .. s
            sys.script.load(path)
            menu.set_mode(false)
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
        menu.redraw()
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



-- PARAMS

p.key[pPARAMS] = function(n,z)
    if n==2 and z==1 then 
        menu.set_page(pHOME)
    end
end

p.enc[pPARAMS] = sys.none

p.redraw[pPARAMS] = function()
    s_clear()
    s_level(10)
    s_move(0,10)
    s_text("params")
end

p.init[pPARAMS] = sys.none


-- SETTINGS
p.set = {}
p.set.pos = 0
p.set.list = {"audio in gain:","headphone gain:", "wifi >"}
p.set.len = 3

p.key[pSETTINGS] = function(n,z)
    if n==2 and z==1 then 
        sys.file.state.save()
        menu.set_page(pHOME)
    elseif n==3 and z==1 and p.set.pos==2 then
        menu.set_page(pWIFI) 
    end
end

p.enc[pSETTINGS] = function(n,delta)
    if n==2 then 
        p.set.pos = p.set.pos + delta 
        p.set.pos = clamp(p.set.pos, 0, p.set.len-1)
	    --if p.set.pos > p.set.len - 1 then p.set.pos = p.set.len - 1
        --elseif p.set.pos < 0 then p.set.pos = 0 end
        menu.redraw()
    elseif n==3 then
        if p.set.pos == 0 then
            sys.input = sys.input + delta
            sys.input = clamp(sys.input,0,63)
            gain_in(sys.input,0) --L
            gain_in(sys.input,1) --R
            menu.redraw()
        elseif p.set.pos == 1 then
            sys.hp = sys.hp + delta
            sys.hp = clamp(sys.hp,0,63)
            gain_hp(sys.hp) 
            menu.redraw()
        end
    end
end

p.redraw[pSETTINGS] = function()
    s_clear()
    s_level(10)
    s_move(0,10)
    s_text("settings")
    for i=3,5 do
       	s_move(0,10*i)
       	line = string.gsub(p.set.list[i-2],'.lua','')
       	if(i==p.set.pos+3) then
           	s_level(15)
       	else
           	s_level(4)
       	end
       	s_text(string.upper(line)) 
    end

    if p.set.pos==0 then s_level(15) else s_level(4) end
    s_move(127,30)
    s_text_right(sys.input)
    if p.set.pos==1 then s_level(15) else s_level(4) end
    s_move(127,40)
    s_text_right(sys.hp)
end

p.init[pSETTINGS] = sys.none


-- SLEEP

p.key[pSLEEP] = function(n,z)
    if n==2 and z==1 then 
        menu.set_page(pHOME)
    elseif n==3 and z==1 then
        print("SLEEP")
        --TODO fade out screen then run the shutdown script
        os.execute("sudo shutdown now")
    end
end

p.enc[pSLEEP] = sys.none

p.redraw[pSLEEP] = function()
    s_clear()
    s_move(48,40)
    s_text("sleep?")
    --TODO do an animation here! fade the volume down
end

p.init[pSLEEP] = sys.none


-- STATUS
p.key[pSTATUS] = function(n,z)
    if n==3 and z==1 then 
        menu.set_page(pHOME)
    elseif n==2 and z==1 then 
        menu.set_page(pLOG)
    end
end

p.enc[pSTATUS] = sys.none

p.redraw[pSTATUS] = function()
    s_clear()
    s_aa(1)

    local status = "battery "..norns.batterypercent 
    if norns.powerpresent==1 then status = status.."+" end 
    local current = os.capture("cat /sys/class/power_supply/bq27441-0/current_now")
    current = tonumber(current) / 1000

    s_level(4)
    s_move(0,8)
    s_text(status .. " > "..current.."mA")
    
    -- draw current script loaded
    s_move(0,16)
    s_level(15)
    s_text(sys.file.state.script)
end

p.init[pSTATUS] = sys.none


-- WIFI
p.wifi = {}
p.wifi.pos = 0
p.wifi.list = {}
p.wifi.len = 3 

p.key[pWIFI] = function(n,z)
    if n==2 and z==1 then
        menu.set_page(pSETTINGS)
    elseif n==3 and z==1 then
        if p.wifi.pos == 0 then
            print "wifi off"
            os.execute("~/norns-image/scripts/wifi.sh off &")
            menu.set_page(pSETTINGS)
        elseif p.wifi.pos == 1 then
            print "wifi on"
            os.execute("~/norns-image/scripts/wifi.sh on &")
            menu.set_page(pSETTINGS)
        else
            print "wifi hotspot"
            os.execute("~/norns-image/scripts/wifi.sh hotspot &")
            menu.set_page(pSETTINGS)
        end
    end
end

p.enc[pWIFI] = function(n,delta)
    if n==2 then 
        p.wifi.pos = p.wifi.pos + delta 
	    if p.wifi.pos > p.wifi.len - 1 then p.wifi.pos = p.wifi.len - 1
        elseif p.wifi.pos < 0 then p.wifi.pos = 0 end
        menu.redraw()
    end
end

p.redraw[pWIFI] = function()
    s_clear()
    s_level(15)
    s_move(0,10)
    --FIXME wifi queries shouldn't be in refresh, called way too often
    local net = ''..os.capture("ifconfig wlan0| grep 'inet ' | awk '{print $2}'")
    if net == '' then net = 'no wifi' 
    else
        net = net .. " at "
        net = net .. os.capture("iw dev wlan0 link | grep 'signal' | awk '{print $2}'")
        net = net .. "dBm"
    end
    s_text(net)

    for i=3,5 do
       	s_move(0,10*i)
       	line = p.wifi.list[i-2]
       	if(i==p.wifi.pos+3) then
           	s_level(15)
       	else
           	s_level(4)
       	end
       	s_text(string.upper(line)) 
     end

end 

p.init[pWIFI] = function()
    ssid = os.capture("cat ~/ssid.wifi") 
    p.wifi.list = {"off","> "..ssid,"hotspot"}
end

-- LOG
p.key[pLOG] = function(n,z)
    if n==3 and z==1 then 
        menu.set_page(pHOME)
    elseif n==2 and z==1 then
        menu.set_page(pSTATUS)
    end
end


p.enc[pLOG] = sys.none

p.redraw[pLOG] = function()
    s_clear()
    s_level(10)
    for i=1,8 do
        s_move(0,i*8)
        s_text(sys.log.get(i))
    end
end

p.init[pLOG] = sys.none

menu.level = function(delta)
    sys.file.state.out = sys.file.state.out + delta
    if sys.file.state.out < 0 then sys.file.state.out = 0 
    elseif sys.file.state.out > 64 then sys.file.state.out = 64 end
    print("level: " .. sys.file.state.out)
    --level_out(norns.state.out,0) 
    --level_out(norns.state.out,1) 
    --level_hp(norns.state.out)
    --level_out(norns.state.out/64)
end 
