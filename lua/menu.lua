-- menu.lua
-- norns screen-based navigation module

local tab = require 'tabutil'
local util = require 'util'
local paramset = require 'paramset'
local menu = {}

-- global functions for scripts
key = norns.none
enc = norns.none
redraw = norns.blank
cleanup = norns.none

-- level enums
local pMIX = 0
local pHOME = 1
local pSELECT = 2
local pPREVIEW = 3
local pPARAMS = 4
local pSYSTEM = 5
local pAUDIO = 6 
local pWIFI = 7
local pSYNC = 8
local pUPDATE = 9
local pLOG = 10
local pSLEEP = 11

-- page pointer
local m = {}
m.key = {}
m.enc = {}
m.redraw = {}
m.init = {}
m.deinit = {}

menu.mode = false
menu.page = pHOME
menu.alt = false
menu.scripterror = false
menu.errormsg = ""

-- mix paramset
mix = paramset.new()
mix:add_number("output",0,64)
mix:set_action("output",
  function(x)
    norns.state.out = x
    norns.audio.output_level(x/64) 
  end)
mix:add_number("input",0,64)
mix:set_action("input",
  function(x)
    norns.state.input = x
    norns.audio.input_level(1,x/64) 
    norns.audio.input_level(2,x/64) 
  end) 
mix:add_number("monitor",0,64)
mix:set_action("monitor",
  function(x)
    norns.state.monitor = x
    norns.audio.monitor_level(x/64) 
  end)
mix:add_option("monitor_mode",{"STEREO","MONO"})
mix:set_action("monitor_mode",
  function(x)
    if x == 1 then
      norns.state.monitor_mode = x
      norns.audio.monitor_stereo()
    else
      norns.state.monitor_mode = x
      norns.audio.monitor_mono()
    end
  end)
mix:add_number("headphone",0,63)
mix:set_action("headphone",
  function(x)
    norns.state.hp = x
    gain_hp(norns.state.hp)
  end)

-- TAPE modes: OUTPUT, OUTPUT+MONITOR, OUTPUT/MONITOR SPLIT
-- TAPE (playback) VOL, SPEED?



local pending = false
-- metro for key hold detection
local metro = require 'metro'
local t = metro[31]
t.time = 0.25
t.count = 1
t.callback = function(stage)
  menu.key(1,1)
  pending = false
end

-- metro for status updates
local u = metro[30]


-- assigns key/enc/screen handlers after user script has loaded
norns.menu = {}
norns.menu.init = function()
  menu.set_mode(menu.mode)
end
norns.menu.status = function() return menu.mode end
norns.menu.set = function(new_enc, new_key, new_redraw)
  menu.penc = new_enc
  menu.key = new_key
  menu.redraw = new_redraw
end
norns.menu.get_enc = function() return menu.penc end
norns.menu.get_key = function() return menu.key end
norns.menu.get_redraw = function() return menu.redraw end

norns.scripterror = function(msg)
  local msg = msg;
  if msg == nil then msg = "" end
  print("### SCRIPT ERROR: "..msg)
  menu.errormsg = msg
  menu.scripterror = true
  menu.set_page(pHOME)
  menu.set_mode(true)
end 

norns.init_done = function(status)
  menu.set_page(pHOME)
  if status == true then
    menu.scripterror = false
    m.params.pos = 0
    menu.set_mode(false)
  end 
end




-- input redirection

menu.enc = function(n, delta)
  if n==1 and menu.alt == false then mix:delta("output",delta)
  else menu.penc(n, delta) end
end


norns.key = function(n, z)
  -- key 1 detect for short press
  if n == 1 then
    if z == 1 then
      menu.alt = true
      pending = true
      t:start()
    elseif z == 0 and pending == true then
      menu.alt = false
      if menu.mode == true and menu.scripterror == false then
        menu.set_mode(false)
      else menu.set_mode(true) end
      t:stop()
      pending = false
    elseif z == 0 then
      menu.alt = false
      menu.key(n,z) -- always 1,0
      if menu.mode == true then menu.redraw() end
    else
      menu.key(n,z) -- always 1,1
    end
    -- key 2/3 pass
  else
    menu.key(n,z)
  end
end

-- menu set mode
menu.set_mode = function(mode)
  if mode == false then
    menu.mode = false
    m.deinit[menu.page]()
    redraw = norns.script.redraw
    menu.key = key
    norns.encoders.callback = enc
    norns.encoders.set_accel(0,true)
    norns.encoders.set_sens(0,1)
    redraw()
  else -- enable menu mode
    menu.mode = true
    menu.alt = false
    redraw = norns.none
    screen.font_face(0)
    screen.font_size(8)
    screen.line_width(1)
    menu.set_page(menu.page)
    norns.encoders.callback = menu.enc
    norns.encoders.set_accel(0,true)
    norns.encoders.set_sens(1,1)
    norns.encoders.set_sens(2,0.5)
    norns.encoders.set_sens(3,0.5)
  end
end

-- set page
menu.set_page = function(page)
  m.deinit[menu.page]()
  menu.page = page
  menu.key = m.key[page]
  menu.penc = m.enc[page]
  menu.redraw = m.redraw[page]
  m.init[page]()
  menu.redraw()
end

-- set monitor level
menu.monitor = function(delta)
  local l = util.clamp(norns.state.monitor + delta,0,64)
  if l ~= norns.state.monitor then
    norns.state.monitor = l
    mix:set("monitor",l)
    --audio_monitor_level(l / 64.0)
  end
end


-- --------------------------------------------------
-- interfaces

-----------------------------------------
-- MIX

m.mix = {}

local tOFF = 0
local tREC = 1
local tPLAY = 2

local tsREC = 0
local tsPAUSE = 1

local tape = {} 
tape.key = false
tape.mode = tOFF 
tape.status = 0
tape.name = ""
tape.time = 0

m.key[pMIX] = function(n,z)
  if n==3 and z==1 and tape.key == false then
    menu.set_page(pHOME)
  elseif n==2 then
    if z==1 then tape.key = true
    else tape.key = false end
  elseif n==3 and tape.key == true and z==1 then
    if tape.mode == tOFF then 
      tape.name = os.date("%y-%m-%d_%H-%M") .. ".aif"
      tape_new(tape.name)
      print("new tape > "..tape.name)
      tape.mode = tREC
      tape.status = tsPAUSE
      redraw()
    elseif tape.mode == tREC and tape.status == tsPAUSE then 
      tape.status = tsREC
      tape_start_rec()
      tape.metro = metro.alloc()
      tape.metro.callback = function() tape.time = tape.time + 1 end
      tape.metro.count = -1
      tape.metro.time = 1
      tape.time = 0 
      tape.metro:start()
    elseif tape.mode == tREC and tape.status == tsREC then
      print("stopping tape")
      tape_stop_rec()
      tape.mode = tOFF 
      tape.metro:stop()
    end 
  end
end

m.enc[pMIX] = function(n,d)
end

m.redraw[pMIX] = function()
  local n
  screen.clear()
  screen.aa(1)
  screen.line_width(1)

  local x = -40
  screen.level(2)
  n = mix:get("output")/64*48
  screen.rect(x+42,56,2,-n)
  screen.stroke()

  screen.level(15)
  n = m.mix.out1/64*48
  screen.rect(x+48,56,2,-n)
  screen.stroke()

  n = m.mix.out1/64*48
  screen.rect(x+54,56,2,-n)
  screen.stroke()

  screen.level(2)
  n = mix:get("input")/64*48
  screen.rect(x+64,56,2,-n)
  screen.stroke()

  screen.level(15)
  n = m.mix.in1/64*48
  screen.rect(x+70,56,2,-n)
  screen.stroke()
  n = m.mix.in2/64*48
  screen.rect(x+76,56,2,-n)
  screen.stroke()

  if menu.alt then screen.level(7) else screen.level(2) end
  n = mix:get("monitor")/64*48
  screen.rect(x+86,56,2,-n)
  screen.stroke()

  --screen.aa(0)
  --screen.line_width(1)
  screen.level(7)

  screen.move(1,62)
  screen.line(3,60)
  screen.line(5,62)
  screen.stroke()

  screen.move(23,60)
  screen.line(25,62)
  screen.line(27,60)
  screen.stroke()

  screen.move(45,61)
  screen.line(49,61)
  screen.stroke()

  if tape.key then screen.level(15) else screen.level(2) end
  screen.move(127,56)
  screen.text_right("TAPE")

  screen.level(10) 
  if tape.mode == tREC then
    screen.move(127,48)
    if tape.status == tsPAUSE then
      screen.text_right("ready")
    elseif tape.status == tsREC then
      screen.text_right("recording")
      screen.move(127,40)
      local min = math.floor(tape.time / 60)
      local sec = tape.time % 60
      screen.text_right(min..":"..sec)
    end 
  end

  screen.level(2)
  screen.move(127,12)
  if menu.alt == false then screen.text_right(norns.battery_percent)
  else screen.text_right(norns.battery_current.."mA") end

  screen.update()
end

m.init[pMIX] = function()
  norns.vu = m.mix.vu
  m.mix.in1 = 0
  m.mix.in2 = 0
  m.mix.out1 = 0
  m.mix.out2 = 0 
end

m.deinit[pMIX] = function()
  norns.vu = norns.none
end

m.mix.vu = function(in1,in2,out1,out2)
  m.mix.in1 = in1
  m.mix.in2 = in2
  m.mix.out1 = out1
  m.mix.out2 = out2
  menu.redraw()
end



-----------------------------------------
-- HOME

m.home = {}
m.home.pos = 0
m.home.list = {"SELECT >", "PARAMETERS >", "SYSTEM >", "SLEEP >"}
m.home.len = 4

m.init[pHOME] = norns.none
m.deinit[pHOME] = norns.none

m.key[pHOME] = function(n,z)
  if n == 2 and z == 1 then
    menu.set_page(pMIX)
  elseif n == 3 and z == 1 then
    local choices = {pSELECT, pPARAMS, pSYSTEM, pSLEEP}
    menu.set_page(choices[m.home.pos+1])
  end
end

m.enc[pHOME] = function(n,delta)
  if n == 2 then
    m.home.pos = m.home.pos + delta
    if m.home.pos > m.home.len - 1 then m.home.pos = m.home.len - 1
    elseif m.home.pos < 0 then m.home.pos = 0 end
    menu.redraw()
  end
end

m.redraw[pHOME] = function()
  screen.clear()
  -- draw current script loaded
  screen.move(0,10)
  screen.level(15)
  local line = string.upper(norns.state.name)
  if(menu.scripterror) then line = line .. " (error: " .. menu.errormsg .. ")" end
  screen.text(line)

  -- draw file list and selector
  for i=3,6 do
    screen.move(0,10*i)
    line = string.gsub(m.home.list[i-2],'.lua','')
    if(i==m.home.pos+3) then
      screen.level(15)
    else
      screen.level(4)
    end
    screen.text(string.upper(line))
  end
  screen.update()
end


----------------------------------------
-- SELECT

m.sel = {}
m.sel.pos = 0
m.sel.list = util.scandir(script_dir)
m.sel.len = tab.count(m.sel.list)
m.sel.depth = 0
m.sel.folders = {}
m.sel.path = ""
m.sel.file = ""

m.sel.dir = function()
  local path = script_dir
  for k,v in pairs(m.sel.folders) do
    path = path .. v
  end
  print("path: "..path)
  return path
end

m.init[pSELECT] = function()
  if m.sel.depth == 0 then
    m.sel.list = util.scandir(script_dir)
  else
    m.sel.list = util.scandir(m.sel.dir())
  end
  m.sel.len = tab.count(m.sel.list)
end

m.deinit[pSELECT] = norns.none

m.key[pSELECT] = function(n,z)
  -- back
  if n==2 and z==1 then
    if m.sel.depth > 0 then
      print('back')
      m.sel.folders[m.sel.depth] = nil
      m.sel.depth = m.sel.depth - 1
      -- FIXME return to folder position
      m.sel.list = util.scandir(m.sel.dir())
      m.sel.len = tab.count(m.sel.list)
      m.sel.pos = 0
      menu.redraw()
    else
      menu.set_page(pHOME)
    end
    -- select
  elseif n==3 and z==1 then
    m.sel.file = m.sel.list[m.sel.pos+1]
    if string.find(m.sel.file,'/') then
      print("folder")
      m.sel.depth = m.sel.depth + 1
      m.sel.folders[m.sel.depth] = m.sel.file
      m.sel.list = util.scandir(m.sel.dir())
      m.sel.len = tab.count(m.sel.list)
      m.sel.pos = 0
      menu.redraw()
    else
      local path = ""
      for k,v in pairs(m.sel.folders) do
        path = path .. v
      end
      m.sel.path = path .. m.sel.file
      menu.set_page(pPREVIEW)
    end
  end
end

m.enc[pSELECT] = function(n,delta)
  -- scroll file list
  if n==2 then
    m.sel.pos = util.clamp(m.sel.pos + delta, 0, m.sel.len - 1)
    menu.redraw()
  end
end

m.redraw[pSELECT] = function()
  -- draw file list and selector
  screen.clear()
  screen.level(15)
  for i=1,6 do
    if (i > 2 - m.sel.pos) and (i < m.sel.len - m.sel.pos + 3) then
      screen.move(0,10*i)
      line = string.gsub(m.sel.list[i+m.sel.pos-2],'.lua','')
      if(i==3) then
        screen.level(15)
      else
        screen.level(4)
      end
      screen.text(string.upper(line))
    end
  end
  screen.update()
end



-----------------------------------------
-- PREVIEW

m.pre = {}
m.pre.meta = {}

m.init[pPREVIEW] = function()
  m.pre.meta = norns.script.metadata(m.sel.path)
  m.pre.len = tab.count(m.pre.meta)
  if m.pre.len == 0 then
    table.insert(m.pre.meta, string.gsub(m.sel.file,'.lua','') .. " (no metadata)")
  end 
  m.pre.state = 0
  m.pre.pos = 0
  m.pre.posmax = m.pre.len - 8
  if m.pre.posmax < 0 then m.pre.posmax = 0 end
end

m.deinit[pPREVIEW] = norns.none

m.key[pPREVIEW] = function(n,z)
  if n==3 and m.pre.state == 1 then
    norns.script.load(m.sel.path)
  elseif n ==3 and z == 1 then
    m.pre.state = 1
  elseif n == 2 and z == 1 then
    menu.set_page(pSELECT)
  end
end

m.enc[pPREVIEW] = function(n,d)
  if n==2 then
    m.pre.pos = util.clamp(m.pre.pos + d, 0, m.pre.posmax)
    menu.redraw()
  end
end 

m.redraw[pPREVIEW] = function()
  screen.clear()
  screen.level(15)
  local i
  for i=1,8 do
    if i <= m.pre.len then
      screen.move(0,i*8-2)
      screen.text(m.pre.meta[i+m.pre.pos])
    end
  end 
  screen.update()
end


-----------------------------------------
-- PARAMS

m.params = {}
m.params.pos = 0

m.key[pPARAMS] = function(n,z)
  if menu.alt then
    if n==2 and z==1 then
      params:read(norns.state.name..".pset")
    elseif n==3 and z==1 then
      params:write(norns.state.name..".pset")
    end 
  elseif n==2 and z==1 then
    menu.set_page(pHOME)
  elseif n==3 and z==1 then
    if params:t(m.params.pos+1) == params.tFILE then
      fileselect.enter(os.getenv("HOME").."/dust", m.params.newfile)
    end
  end
end

m.params.newfile = function(file)
  if file ~= "cancel" then
    params:set(m.params.pos+1,file)
    menu.redraw()
  end
end

m.enc[pPARAMS] = function(n,d)
  if n==2 then
    local prev = m.params.pos
    m.params.pos = util.clamp(m.params.pos + d, 0, params.count - 1)
    if m.params.pos ~= prev then menu.redraw() end
  elseif n==3 and params.count > 0 then
    params:delta(m.params.pos+1,d)
    menu.redraw()
  end
end

m.redraw[pPARAMS] = function()
  screen.clear()
  if(params.count > 0) then 
    if not menu.alt then
      local i
      for i=1,6 do
        if (i > 2 - m.params.pos) and (i < params.count - m.params.pos + 3) then
          if i==3 then screen.level(15) else screen.level(4) end
          local param_index = i+m.params.pos-2

          if params:t(param_index) == params.tSEPARATOR then
            screen.move(0,10*i)
            screen.text(params:string(param_index))
          else
            screen.move(0,10*i)
            screen.text(params:get_name(param_index))
            screen.move(127,10*i)
            screen.text_right(params:string(param_index))
          end
        end 
      end
    else
      screen.move(20,50)
      screen.text("load")
      screen.move(90,50)
      screen.text("save")
    end
  else
    screen.move(0,10)
    screen.level(4)
    screen.text("no parameters")
  end
  screen.update()
end

m.init[pPARAMS] = function()
  u.callback = function() menu.redraw() end
  u.time = 1
  u.count = -1
  u:start()
end

m.deinit[pPARAMS] = function()
  u:stop()
end


-----------------------------------------
-- SYSTEM
m.sys = {}
m.sys.pos = 0
m.sys.list = {"audio > ", "wifi >", "sync >", "update >", "log >"}
m.sys.input = 0
m.sys.disk = ""

m.key[pSYSTEM] = function(n,z)
  if n==2 and z==1 then
    norns.state.save()
    menu.set_page(pHOME)
  elseif n==3 and z==1 and m.sys.pos==0 then
    menu.set_page(pAUDIO) 
  elseif n==3 and z==1 and m.sys.pos==1 then
    menu.set_page(pWIFI)
  elseif n==3 and z==1 and m.sys.pos==4 then
    menu.set_page(pLOG)
  elseif n==1 then
    menu.redraw()
  end
end

m.enc[pSYSTEM] = function(n,delta)
  if n==2 then
    m.sys.pos = m.sys.pos + delta
    m.sys.pos = util.clamp(m.sys.pos, 0, #m.sys.list - 1)
    menu.redraw()
  --[[elseif n==3 then
    if m.sys.pos == 1 then
      if m.sys.input == 0 or m.sys.input == 1 then
        norns.state.input_left = norns.state.input_left + delta
        norns.state.input_left = util.clamp(norns.state.input_left,0,63)
        gain_in(norns.state.input_left,0)
      end
      if m.sys.input == 0 or m.sys.input == 2 then
        norns.state.input_right = norns.state.input_right + delta
        norns.state.input_right = util.clamp(norns.state.input_right,0,63)
        gain_in(norns.state.input_right,1)
      end
      menu.redraw()
    elseif m.sys.pos == 2 then
      norns.state.hp = norns.state.hp + delta
      norns.state.hp = util.clamp(norns.state.hp,0,63)
      gain_hp(norns.state.hp)
      menu.redraw()
    end]]--
  end
end

m.redraw[pSYSTEM] = function()
  screen.clear()

  for i=1,#m.sys.list do
    screen.move(0,10*i+10)
    if(i==m.sys.pos+1) then
      screen.level(15)
    else
      screen.level(4)
    end
    screen.text(string.upper(m.sys.list[i]))
  end

  if m.sys.pos==1 and (m.sys.input == 0 or m.sys.input == 1) then
    screen.level(15) else screen.level(4)
  end

  screen.move(127,30)
  if wifi.state == 2 then m.sys.net = wifi.ip
  else m.sys.net = wifi.status end
  screen.text_right(m.sys.net)

  screen.level(2)
  screen.move(127,40)
  screen.text_right("disk free: "..m.sys.disk)

  screen.move(127,50)
  screen.text_right("v"..norns.version.norns)
  screen.update()
end

m.init[pSYSTEM] = function()
  m.sys.disk = util.os_capture("df -hl | grep '/dev/root' | awk '{print $4}'") 
  u.callback = function()
    m.sysquery()
    menu.redraw()
  end
  u.time = 3
  u.count = -1
  u:start()
end

m.deinit[pSYSTEM] = function()
  u:stop()
end

m.sysquery = function()
  wifi.update()
end






-----------------------------------------
-- WIFI
m.wifi = {}
m.wifi.pos = 0
m.wifi.list = {"off","hotspot","network >"}
m.wifi.len = 3
m.wifi.selected = 1
m.wifi.try = ""

m.key[pWIFI] = function(n,z)
  if n==2 and z==1 then
    menu.set_page(pSYSTEM)
  elseif n==3 and z==1 then
    if m.wifi.pos == 0 then
      print "wifi off"
      wifi.off()
    elseif m.wifi.pos == 1 then
      print "wifi hotspot"
      wifi.hotspot()
    elseif m.wifi.pos == 2 then
      m.wifi.try = wifi.scan_list[m.wifi.selected]
      textentry.enter(m.wifi.passdone, wifi.psk)
    end
  end
end

m.wifi.passdone = function(txt)
  if txt ~= nil then
    os.execute("~/norns/wifi.sh select "..m.wifi.try.." "..txt.." &")
    wifi.on()
  end
  menu.redraw()
end 

m.enc[pWIFI] = function(n,delta)
  if n==2 then
    m.wifi.pos = m.wifi.pos + delta
    if m.wifi.pos > m.wifi.len - 1 then m.wifi.pos = m.wifi.len - 1
    elseif m.wifi.pos < 0 then m.wifi.pos = 0 end
    menu.redraw()
  elseif n==3 and m.wifi.pos == 2 then
    m.wifi.selected = util.clamp(1,m.wifi.selected+delta,wifi.scan_count)
    menu.redraw()
  end
end

m.redraw[pWIFI] = function()
  screen.clear()
  screen.level(15)
  screen.move(0,10)
  if wifi.state == 2 then
    screen.text("status: router "..wifi.ssid)
  else screen.text("status: "..wifi.status) end
  if wifi.state > 0 then
    screen.level(4)
    screen.move(0,20)
    screen.text(wifi.ip)
    if wifi.state == 2 then
      screen.move(127,20)
      screen.text_right(wifi.signal .. "dBm")
    end
  end

  screen.move(0,40+wifi.state*10)
  screen.text("-")

  for i=1,m.wifi.len do
    screen.move(8,30+10*i)
    line = m.wifi.list[i]
    if(i==m.wifi.pos+1) then
      screen.level(15)
    else
      screen.level(4)
    end
    screen.text(string.upper(line))
  end

  screen.move(127,60)
  if m.wifi.pos==2 then screen.level(15) else screen.level(4) end
  if wifi.scan_count > 0 then
    screen.text_right(wifi.scan_list[m.wifi.selected])
  else screen.text_right("NONE") end

  screen.update()
end

m.init[pWIFI] = function()
  wifi.scan()
  wifi.update()
  --m.wifi.selected = wifi.scan_active
  m.wifi.selected = 1
  u.time = 1
  u.count = -1
  u.callback = function()
    wifi.update()
    menu.redraw()
  end
  u:start()
end

m.deinit[pWIFI] = function()
  u:stop()
end


-----------------------------------------
-- AUDIO

m.audio = {}
m.audio.pos = 0

m.key[pAUDIO] = function(n,z)
  if menu.alt then
    if n==2 and z==1 then
      mix:read(norns.state.name..".pset")
    elseif n==3 and z==1 then
      mix:write(norns.state.name..".pset")
    end 
  elseif n==2 and z==1 then
    menu.set_page(pHOME)
  elseif n==3 and z==1 then
    if mix:t(m.audio.pos+1) == mix.tFILE then
      fileselect.enter(os.getenv("HOME").."/dust", m.audio.newfile)
    end
  end
end

m.audio.newfile = function(file)
  if file ~= "cancel" then
    mix:set(m.audio.pos+1,file)
    menu.redraw()
  end
end

m.enc[pAUDIO] = function(n,d)
  if n==2 then
    local prev = m.audio.pos
    m.audio.pos = util.clamp(m.audio.pos + d, 0, mix.count - 1)
    if m.audio.pos ~= prev then menu.redraw() end
  elseif n==3 then
    mix:delta(m.audio.pos+1,d)
    menu.redraw()
  end
end

m.redraw[pAUDIO] = function()
  screen.clear()
  local i
  for i=1,6 do
    if (i > 2 - m.audio.pos) and (i < mix.count - m.audio.pos + 3) then
      if i==3 then screen.level(15) else screen.level(4) end
      local param_index = i+m.audio.pos-2

      if mix:t(param_index) == mix.tSEPARATOR then
        screen.move(0,10*i)
        screen.text(mix:string(param_index))
      else
        screen.move(0,10*i)
        screen.text(mix:get_name(param_index))
        screen.move(127,10*i)
        screen.text_right(mix:string(param_index))
      end
    end 
  end
  screen.update()
end

m.init[pAUDIO] = function()
  u.callback = function() menu.redraw() end
  u.time = 1
  u.count = -1
  u:start()
end

m.deinit[pAUDIO] = function()
  u:stop()
end




-----------------------------------------
-- LOG
m.log = {}
m.log.pos = 0

m.key[pLOG] = function(n,z)
  if n==2 and z==1 then
    menu.set_page(pSYSTEM)
  elseif n==3 and z==1 then
    m.log.pos = 0
    menu.redraw()
  end
end

m.enc[pLOG] = function(n,delta)
  if n==2 then
    m.log.pos = util.clamp(m.log.pos+delta, 0, math.max(norns.log.len()-7,0))
    menu.redraw()
  end
end

m.redraw[pLOG] = function()
  screen.clear()
  screen.level(10)
  for i=1,8 do
    screen.move(0,(i*8)-1)
    screen.text(norns.log.get(i+m.log.pos))
  end
  screen.update()
end

m.init[pLOG] = function()
  m.log.pos = 0
  u.time = 1
  u.count = -1
  u.callback = menu.redraw
  u:start()
end

m.deinit[pLOG] = function()
  u:stop()
end 


-----------------------------------------
-- SLEEP

m.key[pSLEEP] = function(n,z)
  if n==2 and z==1 then
    menu.set_page(pHOME)
  elseif n==3 and z==1 then
    print("SLEEP")
    --TODO fade out screen then run the shutdown script
    mix:set("output",0)
    --norns.audio.set_audio_level(0)
    wifi.off()
    os.execute("sleep 0.5; sudo shutdown now")
  end
end

m.enc[pSLEEP] = norns.none

m.redraw[pSLEEP] = function()
  screen.clear()
  screen.move(48,40)
  screen.text("sleep?")
  --TODO do an animation here! fade the volume down
  screen.update()
end

m.init[pSLEEP] = norns.none
m.deinit[pSLEEP] = norns.none


