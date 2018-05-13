-- menu.lua
-- norns screen-based navigation module
local tab = require 'tabutil'
local util = require 'util'
local screen = require 'screen'
local menu = {}

-- global functions for scripts
key = norns.none
enc = norns.none
redraw = norns.blank
cleanup = norns.none

-- level enums
local pHOME = 1
local pSELECT = 2
local pPREVIEW = 3
local pAUDIO = 4
local pPARAMS = 5
local pSYSTEM = 6
local pWIFI = 7
local pSLEEP = 8
local pLOG = 9

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

local pending = false
-- metro for key hold detection
local metro = require 'metro'
local t = metro[31]
t.time = 0.25
t.count = 2
t.callback = function(stage)
  if(stage == 2) then
    if(menu.mode == true) then
      menu.alt = true
      --menu.redraw()
    end
    menu.key(1,1)
    pending = false
  end
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
  if n==1 and menu.alt == false then menu.level(delta)
  elseif n==1 and menu.alt == true then menu.monitor(delta)
  else menu.penc(n, delta) end
end


norns.key = function(n, z)
  -- key 1 detect for short press
  if n == 1 then
    if z == 1 then
      pending = true
      t:start()
    elseif z == 0 and pending == true then
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

-- set audio level
menu.level = function(delta)
  norns.audio.adjust_output_level(delta)
end

-- set monitor level
menu.monitor = function(delta)
  local l = util.clamp(norns.state.monitor + delta,0,64)
  if l ~= norns.state.monitor then
    norns.state.monitor = l
    audio_monitor_level(l / 64.0)
  end
end

-- --------------------------------------------------
-- interfaces

-- HOME

m.home = {}
m.home.pos = 0
m.home.list = {"SELECT >", "PARAMETERS >", "SYSTEM >", "SLEEP >"}
m.home.len = 4

m.init[pHOME] = norns.none
m.deinit[pHOME] = norns.none

m.key[pHOME] = function(n,z)
  if n == 2 and z == 1 then
    menu.set_page(pAUDIO)
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


-- SYSTEM
m.sys = {}
m.sys.pos = 0
m.sys.list = {"wifi >", "input gain:","headphone gain:", "log >"}
m.sys.len = 4
m.sys.input = 0
m.sys.disk = ""

m.key[pSYSTEM] = function(n,z)
  if n==2 and z==1 then
    norns.state.save()
    menu.set_page(pHOME)
  elseif n==3 and z==1 and m.sys.pos==3 then
    menu.set_page(pLOG)
  elseif n==3 and z==1 and m.sys.pos==1 then
    m.sys.input = (m.sys.input + 1) % 3
    menu.redraw()
  elseif n==3 and z==1 and m.sys.pos==0 then
    menu.set_page(pWIFI)
  elseif n==1 then
    menu.redraw()
  end
end

m.enc[pSYSTEM] = function(n,delta)
  if n==2 then
    m.sys.pos = m.sys.pos + delta
    m.sys.pos = util.clamp(m.sys.pos, 0, m.sys.len-1)
    --if m.sys.pos > m.sys.len - 1 then m.sys.pos = m.sys.len - 1
    --elseif m.sys.pos < 0 then m.sys.pos = 0 end
    menu.redraw()
  elseif n==3 then
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
    end
  end
end

m.redraw[pSYSTEM] = function()
  screen.clear()
  screen.level(4)
  screen.move(127,10)
  if not menu.alt then
    local pwr = ''
    if norns.powerpresent==1 then pwr="+" end
    screen.text_right("disk: "..m.sys.disk.." / bat: "..norns.battery_percent..pwr)
  else
    screen.text_right(norns.battery_current.."mA")
  end

  for i=1,m.sys.len do
    screen.move(0,10*i+20)
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
  screen.move(101,40)
  if(norns.state.input_left == 0) then screen.text_right("m")
  else screen.text_right(norns.state.input_left - 48) end -- show 48 as unity (0)
  if m.sys.pos==1 and (m.sys.input == 0 or m.sys.input == 2) then
    screen.level(15) else screen.level(4)
  end
  screen.move(127,40)
  if(norns.state.input_right == 0) then screen.text_right("m")
  else screen.text_right(norns.state.input_right - 48) end
  if m.sys.pos==2 then screen.level(15) else screen.level(4) end
  screen.move(127,50)
  screen.text_right(norns.state.hp)
  screen.level(4)
  screen.move(127,30)
  if wifi.state == 2 then
    if not menu.alt then m.sys.net = wifi.ip
    else m.sys.net = wifi.signal .. "dBm" end
  else
    m.sys.net = wifi.status
  end
  screen.text_right(m.sys.net)
  screen.move(127,60)
  screen.text_right("norns v"..norns.version.norns)
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




-- SLEEP

m.key[pSLEEP] = function(n,z)
  if n==2 and z==1 then
    menu.set_page(pHOME)
  elseif n==3 and z==1 then
    print("SLEEP")
    --TODO fade out screen then run the shutdown script
    norns.audio.set_audio_level(0)
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


-- AUDIO
m.audio = {}

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

m.key[pAUDIO] = function(n,z)
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
    elseif tape.mode == tREC and tape.status == tsREC then
      print("stopping tape")
      tape_stop_rec()
      tape.mode = tOFF
    end
  end
end

m.enc[pAUDIO] = function(n,d)
end

m.redraw[pAUDIO] = function()
  screen.clear()
  screen.aa(1)

  screen.line_width(1)
  screen.level(2)
  screen.move(0,64-norns.state.out)
  screen.line(0,63)
  screen.stroke()

  screen.level(15)
  screen.move(3,63)
  screen.line(3,63-m.audio.out1)
  screen.move(6,63)
  screen.line(6,63-m.audio.out2)
  screen.stroke()

  screen.level(2)
  screen.move(13,64-norns.state.monitor)
  screen.line(13,63)
  screen.stroke()

  screen.level(15)
  screen.move(16,63)
  screen.line(16,63-m.audio.in1)
  screen.move(19,63)
  screen.line(19,63-m.audio.in2)
  screen.stroke()

  if menu.alt then
    screen.level(2)
    screen.move(127,53)
    screen.text_right("ALT")
  end

  if tape.key then
    screen.level(2)
    screen.move(127,63)
    screen.text_right("TAPE")
  end

  if tape.mode == tREC then
    screen.move(127,10)
    if tape.status == tsPAUSE then
      screen.text_right("ready")
    elseif tape.status == tsREC then
      screen.text_right("recording")
    end
  end

  --[[if norns.powerpresent==0 then
    screen.level(2)
    screen.move(24,63)
    screen.text("99") -- add batt percentage
  end]]--

  screen.update()
end

m.init[pAUDIO] = function()
  norns.vu = m.audio.vu
  m.audio.in1 = 0
  m.audio.in2 = 0
  m.audio.out1 = 0
  m.audio.out2 = 0
end

m.deinit[pAUDIO] = function()
  norns.vu = norns.none
end

m.audio.vu = function(in1,in2,out1,out2)
  m.audio.in1 = in1
  m.audio.in2 = in2
  m.audio.out1 = out1
  m.audio.out2 = out2
  menu.redraw()
end



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
    screen.move(127,20)
    screen.text_right(wifi.signal .. "dBm")
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
  m.wifi.selected = wifi.scan_active
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
