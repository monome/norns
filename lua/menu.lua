-- menu.lua
-- norns screen-based navigation module

local tab = require 'tabutil'
local util = require 'util'
local paramset = require 'paramset'
local cs = require 'controlspec'
local audio = require 'audio'
local menu = {}

mix = require 'mix'

-- global functions for scripts
key = norns.none
enc = norns.none
redraw = norns.blank
cleanup = norns.none

-- tuning
local KEY1_HOLD_TIME = 0.25

-- level enums
local pHOME = 1
local pSELECT = 2
local pPREVIEW = 3
local pPARAMS = 4
local pSYSTEM = 5
local pAUDIO = 6
local pDEVICES = 7
local pWIFI = 8
local pRESET = 9
local pSLEEP = 10
local pTAPE = 11
local pMIX = 12

-- page pointer
local m = {}
m.key = {}
m.enc = {}
m.redraw = {}
m.init = {}
m.deinit = {}

menu.mode = false
menu.page = pHOME
menu.panel = 3
menu.panels = {pMIX, pTAPE, pHOME, pPARAMS}
menu.alt = false
menu.scripterror = false
menu.locked = true
menu.errormsg = ""
menu.shownav = false
menu.showstats = false


---- METROS
local pending = false
-- metro for key hold detection
local metro = require 'metro'
local t = metro[30]
t.time = KEY1_HOLD_TIME
t.count = 1
t.callback = function(stage)
  menu.key(1,1)
  pending = false
end
-- metro for page status updates
local u = metro[31]
-- metro for tape
local tape_play_counter = metro[32]
local tape_rec_counter = metro[33]
-- metro for nav vanish
local nav_vanish = metro[34]
nav_vanish.time = 1
nav_vanish.callback = function()
  menu.shownav = false
  if menu.mode == true then menu.redraw() end
  nav_vanish:stop()
end
-- screen.lua has metro[35] for screensaver

-- assigns key/enc/screen handlers after user script has loaded
norns.menu = {}
norns.menu.init = function() menu.set_mode(menu.mode) end -- used by fileselect.lua
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
  menu.locked = true
  menu.set_page(pHOME)
  menu.set_mode(true)
end

norns.init_done = function(status)
  menu.set_page(pHOME)
  if status == true then
    menu.scripterror = false
    m.params.pos = 0
    if norns.script.nointerface == true then
      --menu.locked = true
      --menu.set_page(pPARAMS)
      menu.locked = false
      menu.set_mode(false)
    else
      menu.locked = false
      menu.set_mode(false)
    end
  end
  m.params.init_map()
  m.params.read(norns.state.folder_name..".pmap")
end




-- input redirection

menu.enc = function(n, delta)
  if n==1 and menu.alt == false then
    --mix:delta("output",delta)
    local c = util.clamp(menu.panel+delta,1,4)
    if c ~= menu.panel then
      menu.shownav = true
      menu.panel = c
      menu.set_page(menu.panels[menu.panel])
      nav_vanish:start()
    end
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
      if menu.mode == true and menu.locked == false then
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
  if mode == false then -- PLAY MODE
    if menu.mode == true then s_restore() end
    menu.mode = false
    m.deinit[menu.page]()
    redraw = norns.script.redraw
    menu.key = key
    norns.encoders.callback = enc
    norns.encoders.set_accel(0,false)
    norns.encoders.set_sens(0,1)
    redraw()
  else -- MENU MODE
    if menu.mode == false then s_save() end
    menu.mode = true
    menu.alt = false
    redraw = norns.none
    screen.font_face(0)
    screen.font_size(8)
    screen.line_width(1)
    norns.encoders.callback = menu.enc
    norns.encoders.set_accel(1,false)
    norns.encoders.set_sens(1,8)
    norns.encoders.set_accel(2,false)
    norns.encoders.set_sens(2,2)
    norns.encoders.set_accel(3,true)
    norns.encoders.set_sens(3,2)
    menu.set_page(menu.page)
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

-- draw panel indicator
function menu.draw_panel()
  if menu.shownav then
    screen.aa(1)
    screen.line_width(1)
    for i = 1,4 do
      screen.level(i == menu.panel and 8 or 2)
      screen.move((i-1)*33,0)
      screen.line_rel(30,0)
      screen.stroke()
    end
  end
end




-- --------------------------------------------------
-- interfaces

-----------------------------------------
-- HOME

m.home = {}
m.home.pos = 1
m.home.list = {"SELECT >", "SYSTEM >", "SLEEP >"}

m.init[pHOME] = function()
  u.time = 1
  u.count = -1
  u.callback = function() menu.redraw() end
  u:start()
end
m.deinit[pHOME] = function()
  u:stop()
end

m.key[pHOME] = function(n,z)
  if n == 2 and z == 1 then
    menu.showstats = not menu.showstats
    menu.redraw()
  elseif n == 3 and z == 1 then
    local choices = {pSELECT, pSYSTEM, pSLEEP}
    if m.home.pos == 2 then m.sel.depth = 0 end -- reset folder position to root
    menu.set_page(choices[m.home.pos])
  end
end

m.enc[pHOME] = function(n,delta)
  if n == 2 then
    m.home.pos = util.clamp(m.home.pos + delta, 1, #m.home.list)
    menu.redraw()
  end
end

m.redraw[pHOME] = function()
  screen.clear()

  menu.draw_panel()

  -- draw file list and selector
  for i=1,3 do
    screen.move(0,25+10*i)
    line = string.gsub(m.home.list[i],'.lua','')
    if(i==m.home.pos) then
      screen.level(15)
    else
      screen.level(4)
    end
    screen.text(string.upper(line))
  end

  if not menu.showstats then
    screen.move(0,15)
    screen.level(15)
    local line = string.upper(norns.state.name)
    --if(menu.scripterror and state.script ~= '') then
    if(menu.scripterror and menu.errormsg ~= 'NO SCRIPT') then
      line = line .. " (error: " .. menu.errormsg .. ")"
    end
    screen.text(line)
  else
    screen.level(1)
    screen.move(0,10)
    screen.text("BAT " .. norns.battery_percent)
    screen.move(36,10)
    screen.text(norns.battery_current .. "mA")
    screen.move(127,10)
    screen.text_right("DISK " .. norns.disk .. "M")
    screen.move(0,20)
    screen.text("CPU " .. norns.cpu .. "%")
    screen.move(36,20)
    screen.text(norns.temp .. "c")
    screen.move(127,20)
    screen.text_right("IP "..wifi.ip)
    if wifi.state > 0 then
      screen.text_right(wifi.ip)
    end
    screen.move(127,45)
    screen.text_right(norns.version.update)

    screen.level(15)
    screen.move(127,35)
    screen.text_right(string.upper(norns.state.name))
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
m.sel.folderpos = {}
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
      m.sel.folders[m.sel.depth] = nil
      m.sel.depth = m.sel.depth - 1
      m.sel.list = util.scandir(m.sel.dir())
      m.sel.len = tab.count(m.sel.list)
      m.sel.pos = m.sel.folderpos[m.sel.depth] or 0
      menu.redraw()
    else
      menu.set_page(pHOME)
    end
    -- select
  elseif n==3 and z==1 then
    m.sel.file = m.sel.list[m.sel.pos+1]
    if string.find(m.sel.file,'/') then
      m.sel.folderpos[m.sel.depth] = m.sel.pos
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
m.params.n = 0
m.params.loadable = true
m.params.altpos = 1
m.params.map = {}
m.params.init_map = function()
  for i = 1,params.count do m.params.map[i] = -1 end
end

m.key[pPARAMS] = function(n,z)
  if menu.alt then
    if n==3 and z==1 then
      if m.params.altpos == 1 then
        if m.params.n == 0 then
          params:read(norns.state.folder_name..".pset")
        else
          params:read(norns.state.folder_name.."-"..string.format("%02d",m.params.n)..".pset")
        end
        m.params.action = 15
        m.params.action_text = "loaded"
      elseif m.params.altpos == 2 then
        if m.params.n == 0 then
          params:write(norns.state.folder_name..".pset")
        else
          params:write(norns.state.folder_name.."-"..string.format("%02d",m.params.n)..".pset")
        end
        m.params.action = 15
        m.params.action_text = "saved"
        m.params.loadable = true
      end
      menu.redraw()
    end
  elseif n==2 and z==1 then
    --NOT USED
    --menu.set_page(pHOME)
  elseif n==3 and z==1 then
    if not m.params.midimap then
      if params.count > 0 then
        if params:t(m.params.pos+1) == params.tFILE then
          fileselect.enter(os.getenv("HOME").."/dust", m.params.newfile)
        elseif params:t(m.params.pos+1) == params.tTRIGGER then
          params:set(m.params.pos+1)
          m.params.triggered[m.params.pos+1] = 2
        end
      else
        m.params.midilearn = not m.params.midilearn
      end
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
  if menu.alt then
    if n == 2 then
      m.params.altpos = util.clamp(m.params.altpos+d, 1, 3)
      menu.redraw()
    elseif n==3 then
      if m.params.altpos < 3 then
        m.params.n = util.clamp(m.params.n + d,0,100)
        local path
        local f
        if m.params.n == 0 then
          path = data_dir..norns.state.folder_name..".pset"
          f=io.open(path,"r")
        else
          path =data_dir..norns.state.folder_name.."-"..string.format("%02d",m.params.n)..".pset"
          f=io.open(path ,"r")
        end
        --print("pset: "..path)
        if f~=nil then
          m.params.loadable = true
          io.close(f)
        else
          m.params.loadable = false
        end
        menu.redraw()
      else
        m.params.midimap = d > 0
        menu.redraw()
      end
    end
  elseif n==2 then
    local prev = m.params.pos
    m.params.pos = util.clamp(m.params.pos + d, 0, params.count - 1)
    if m.params.pos ~= prev then menu.redraw() end
  elseif n==3 and params.count > 0 then
    if not m.params.midimap then
      params:delta(m.params.pos+1,d)
      menu.redraw()
    else
      m.params.map[m.params.pos+1] = util.clamp(m.params.map[m.params.pos+1]+d,-1,127)
      menu.redraw()
    end
  end
end

m.redraw[pPARAMS] = function()
  screen.clear()

  menu.draw_panel()

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
            if m.params.midimap then
              if params:t(param_index) == params.tCONTROL or params:t(param_index) == params.tTAPER then
                screen.move(127,10*i)
                if m.params.map[param_index] >= 0 then
                  screen.text_right(m.params.map[param_index])
                else
                  screen.text_right("-")
                end
              end
            else
              screen.move(127,10*i)
              if params:t(param_index) ==  params.tTRIGGER then
                if m.params.triggered[param_index] and m.params.triggered[param_index] > 0 then
                  screen.rect(124, 10 * i - 4, 3, 3)
                  screen.fill()
                end
              else
                screen.text_right(params:string(param_index))
              end
            end
          end
        end
      end
      if m.params.midilearn then
        screen.level(15)
        screen.move(80,30)
        screen.text("(learn)")
      end
    else -- menu.alt == true -- param save/load

      screen.level((m.params.altpos == 1) and 15 or 4)
      screen.move(0,30)
      screen.text("load")
      if m.params.altpos == 1 then
        screen.move(127,30)
        screen.level(m.params.loadable and 10 or 1)
        if m.params.n == 0 then
          screen.text_right("default")
        else
          screen.text_right(string.format("%02d",m.params.n))
        end
      end

      screen.level((m.params.altpos == 2) and 15 or 4)
      screen.move(0,40)
      screen.text("save")
      if m.params.altpos == 2 then
        screen.move(127,40)
        screen.level(m.params.loadable and 10 or 4)
        if m.params.n == 0 then
          screen.text_right("default")
        else
          screen.text_right(string.format("%02d",m.params.n))
        end
      end

      screen.level((m.params.altpos == 3) and 15 or 4)
      screen.move(0,50)
      screen.text("midi-cc mapping")
      screen.move(127,50)
      screen.text_right(m.params.midimap and "on" or "off")

      screen.move(0,10)
      screen.level(m.params.action)
      screen.text(m.params.action_text)
    end
  else
    screen.move(0,10)
    screen.level(4)
    screen.text("no parameters")
  end
  screen.update()
end

m.init[pPARAMS] = function()
  m.params.midimap = false
  m.params.midilearn = false
  m.params.action_text = ""
  m.params.action = 0
  m.params.triggered = {}
  u.callback = function()
    if m.params.action > 0 then m.params.action = m.params.action - 1 end
    for k, v in pairs(m.params.triggered) do
      if v > 0 then m.params.triggered[k] = v - 1 end
    end
    menu.redraw()
  end
  u.time = 0.2
  u.count = -1
  u:start()
end

m.deinit[pPARAMS] = function()
  if state.script ~= '' then
    m.params.write(norns.state.folder_name..".pmap")
  end
  m.params.midilearn = false
  u:stop()
end

norns.menu_midi_event = function(data)
  if data[1] == 176 then -- cc
    if m.params.midilearn then
      if params:t(m.params.pos+1) == params.tCONTROL or params:t(m.params.pos+1) == params.tTAPER then
        m.params.map[m.params.pos+1] = data[2]
        menu.redraw()
      end
      m.params.midilearn = false
    else
      local p = tab.key(m.params.map,data[2])
      if p then
        params:set_raw(p,data[3]/127)
      end
      --print(data[2] .. " " .. data[3])
    end
  end
end

function m.params.write(filename)
  local function quote(s)
    return '"'..s:gsub('"', '\\"')..'"'
  end
  -- check for subfolder in filename, create subfolder if it doesn't exist
  local subfolder, found = string.gsub(filename,"/(.*)","")
  if found==1 then
    local fd = io.open(data_dir..subfolder,"r")
    if fd then
      io.close(fd)
    else
      print("creating subfolder")
      os.execute("mkdir "..data_dir..subfolder)
    end
  end
  -- write file
  local fd = io.open(data_dir..filename, "w+")
  io.output(fd)
  for k,v in pairs(m.params.map) do
    io.write(string.format("%s: %d\n", quote(tostring(k)), v))
  end
  io.close(fd)
end

function m.params.read(filename)
  local function unquote(s)
    return s:gsub('^"', ''):gsub('"$', ''):gsub('\\"', '"')
  end
  print("READING PMAP")
  local fd = io.open(data_dir..filename, "r")
  if fd then
    io.close(fd)
    for line in io.lines(data_dir..filename) do
      --local name, value = string.match(line, "(\".-\")%s*:%s*(.*)")
      local name, value = string.match(line, "(\".-\")%s*:%s*(.*)")

      if name and value then
        --print(unquote(name) .. " : " .. value)
        m.params.map[tonumber(unquote(name),10)] = tonumber(value)
      end
    end
  else
    --print("m.params.read: "..filename.." not read.")
  end
end



-----------------------------------------
-- SYSTEM
m.sys = {}
m.sys.pos = 1
m.sys.list = {"AUDIO > ", "DEVICES > ", "WIFI >", "RESET AUDIO"}
m.sys.pages = {pAUDIO, pDEVICES, pWIFI, pRESET}
m.sys.input = 0

m.key[pSYSTEM] = function(n,z)
  if n==2 and z==1 then
    norns.state.save()
    menu.set_page(pHOME)
  elseif n==3 and z==1 then
    menu.set_page(m.sys.pages[m.sys.pos])
  end
end

m.enc[pSYSTEM] = function(n,delta)
  if n==2 then
    m.sys.pos = util.clamp(m.sys.pos + delta, 1, #m.sys.list)
    menu.redraw()
  end
end

m.redraw[pSYSTEM] = function()
  screen.clear()

  for i=1,#m.sys.list do
    screen.move(0,10+10*i)
    if(i==m.sys.pos) then
      screen.level(15)
    else
      screen.level(4)
    end
    screen.text(m.sys.list[i])
  end

  screen.update()
end

m.init[pSYSTEM] = norns.none
m.deinit[pSYSTEM] = norns.none


-----------------------------------------
-- DEVICES
m.devices = {}
m.devices.pos = 1
m.devices.list = {"midi", "grid", "arc"}
m.devices.len = #m.devices.list
function m.devices.refresh()
  m.devices.options = {
    midi = {"none"},
    grid = {"none"},
    arc = {"none"},
  }
  -- create midi list
  for _,i in pairs(midi.list) do
    table.insert(m.devices.options.midi,i)
  end
  for _,i in pairs(grid.list) do
    table.insert(m.devices.options.grid,i)
  end
  for _,i in pairs(arc.list) do
    table.insert(m.devices.options.arc,i)
  end
end

m.key[pDEVICES] = function(n,z)
  if m.devices.mode == "type" then
    if n==2 and z==1 then
      norns.state.save()
      menu.set_page(pSYSTEM)
    elseif n==3 and z==1 then
      m.devices.section = m.devices.list[m.devices.pos]
      m.devices.mode = "list"
      m.devices.len = 4
      m.devices.pos = 1
      menu.redraw()
    end
  elseif m.devices.mode == "list" then
    if n==2 and z==1 then
      m.devices.mode = "type"
      m.devices.len = #m.devices.list
      m.devices.pos = 1
      menu.redraw()
    elseif n==3 and z==1 then
      m.devices.refresh()
      m.devices.mode = "select"
      m.devices.setpos = m.devices.pos
      m.devices.len = #m.devices.options[m.devices.section]
      --tab.print(m.devices.options[m.devices.section])
      m.devices.pos = 1
      menu.redraw()
    end
  elseif m.devices.mode == "select" then
    if n==2 and z==1 then
      m.devices.mode = "list"
      m.devices.len = 4
      m.devices.pos = 1
      menu.redraw()
    elseif n==3 and z==1 then
      local s = m.devices.options[m.devices.section][m.devices.pos]
      if m.devices.section == "midi" then
        midi.vport[m.devices.setpos].name = s
        midi.update_devices()
      elseif m.devices.section == "grid" then
        grid.vport[m.devices.setpos].name = s
        grid.update_devices()
      elseif m.devices.section == "arc" then
        arc.vport[m.devices.setpos].name = s
        arc.update_devices()
      end
      m.devices.mode = "list"
      m.devices.len = 4
      m.devices.pos = 1
      menu.redraw()
    end
  end
end

m.enc[pDEVICES] = function(n,delta)
  if n==2 then
    m.devices.pos = util.clamp(m.devices.pos + delta, 1, m.devices.len)
    menu.redraw()
  end
end

m.redraw[pDEVICES] = function()
  screen.clear()
  for i=1,m.devices.len do
    screen.move(0,10*i+20)
    if(i==m.devices.pos) then
      screen.level(15)
    else
      screen.level(4)
    end
    if m.devices.mode == "type" then
      screen.text(string.upper(m.devices.list[i]) .. " >")
    elseif m.devices.mode == "list" then
      if m.devices.section == "midi" then
        screen.text(i .. ". " .. midi.vport[i].name)
      elseif m.devices.section == "grid" then
        screen.text(i .. ". " .. grid.vport[i].name)
      elseif m.devices.section == "arc" then
        screen.text(i .. ". " .. arc.vport[i].name)
      end
    elseif m.devices.mode == "select" then
      screen.text(m.devices.options[m.devices.section][i])
    end
  end
  screen.update()
end

m.init[pDEVICES] = function()
  m.devices.pos = 1
  m.devices.mode = "type"
  m.devices.len = #m.devices.list
end

m.deinit[pDEVICES] = function() end




-----------------------------------------
-- WIFI
m.wifi = {}
m.wifi.pos = 0
m.wifi.list = {"off","hotspot","network >"}
m.wifi.len = 3
m.wifi.selected = 1
m.wifi.try = ""
m.wifi.countdown = -1

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
    os.execute("~/norns/wifi.sh select \""..m.wifi.try.."\" \""..txt.."\" &")
    os.execute("sudo systemctl stop norns-crone.service")
    wifi.on()
    m.wifi.countdown = 4
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

  if m.wifi.countdown == -1 then
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

  else -- countdown
    screen.move(64,40)
    screen.text_center("disabling audio")
    screen.move(64,50)
    screen.text_center("reset in system menu")
  end
  screen.update()
end

m.init[pWIFI] = function()
  m.wifi.countdown = -1
  wifi.scan()
  wifi.update()
  --m.wifi.selected = wifi.scan_active
  m.wifi.selected = 1
  u.time = 1
  u.count = -1
  u.callback = function()
    if m.wifi.countdown > 0 then m.wifi.countdown = m.wifi.countdown - 1
    elseif m.wifi.countdown == 0 then
      print("wifi timeout")
      m.wifi.countdown = -1
      norns.startup_status.timeout()
      menu.redraw()
    else
      wifi.update()
      menu.redraw()
    end
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
  if n==2 and z==1 then
    menu.set_page(pSYSTEM)
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
-- RESET
m.reset = {}
m.reset.countdown = 0

m.key[pRESET] = function(n,z) end

m.enc[pRESET] = function(n,delta) end

m.redraw[pRESET] = function()
  screen.clear()
  screen.level(10)
  screen.move(64,40)
  screen.text_center("resetting audio")
  screen.update()
end

m.init[pRESET] = function()
  m.reset.countdown = 9
  norns.script.clear()
  os.execute("sudo systemctl restart norns-crone.service")
  u.time = 1
  u.count = -1
  u.callback = function()
    m.reset.countdown = m.reset.countdown - 1
    if m.reset.countdown == 0 then
      menu.set_page(pSYSTEM)
      norns.startup_status.ok()
    end
  end
  u:start()
end

m.deinit[pRESET] = function()
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
    m.sleep = true
    menu.redraw()
    norns.state.clean_shutdown = true
    norns.state.save()
    cleanup()
    if m.tape.rec.sel == TAPE_REC_STOP then audio.tape_record_stop() end
    audio.level_dac(0)
    audio.headphone_gain(0)
    wifi.off()
    os.execute("sleep 0.5; sudo shutdown now")
  end
end

m.enc[pSLEEP] = norns.none

m.redraw[pSLEEP] = function()
  screen.clear()
  screen.move(48,40)
  if m.sleep then
	screen.level(1)
	screen.text("sleep.")
  else
	screen.level(15)
  	screen.text("sleep?")
  end
  --TODO do an animation here! fade the volume down
  screen.update()
end

m.init[pSLEEP] = norns.none
m.deinit[pSLEEP] = norns.none


-----------------------------------------
-- MIX

m.mix = {}
m.mix.sel = 1

m.key[pMIX] = function(n,z)
  if n==2 and z==1 then
    m.mix.sel = (m.mix.sel==1) and 2 or 1
  end
  -- KEY3 not used
end

m.enc[pMIX] = function(n,d)
  if n==2 then
    if m.mix.sel==1 then
      mix:delta("output",d)
    else
      mix:delta("monitor",d)
    end
  elseif n==3 then
    if m.mix.sel==1 then
      mix:delta("input",d)
    else
      mix:delta("tape",d)
    end
  end
end

m.redraw[pMIX] = function()
  local n
  screen.clear()
  screen.aa(1)
  screen.line_width(1)

  menu.draw_panel()

  local x = -40
  screen.level(2)
  n = mix:get_raw("output")*48
  screen.rect(x+42.5,55.5,2,-n)
  screen.stroke()

  screen.level(15)
  n = m.mix.out1/64*48
  screen.rect(x+48.5,55.5,2,-n)
  screen.stroke()

  n = m.mix.out2/64*48
  screen.rect(x+54.5,55.5,2,-n)
  screen.stroke()

  screen.level(2)
  n = mix:get_raw("input")*48
  screen.rect(x+64.5,55.5,2,-n)
  screen.stroke()

  screen.level(15)
  n = m.mix.in1/64*48
  screen.rect(x+70.5,55.5,2,-n)
  screen.stroke()
  n = m.mix.in2/64*48
  screen.rect(x+76.5,55.5,2,-n)
  screen.stroke()

  screen.level(2)
  n = mix:get_raw("monitor")*48
  screen.rect(x+86.5,55.5,2,-n)
  screen.stroke()

  screen.level(2)
  n = mix:get_raw("tape")*48
  screen.rect(x+108.5,55.5,2,-n)
  screen.stroke()

  screen.level(m.mix.sel==1 and 15 or 1)
  screen.move(2,63)
  screen.text("out")
  screen.move(24,63)
  screen.text("in")
  screen.level(m.mix.sel==2 and 15 or 1)
  screen.move(46,63)
  screen.text("mon")
  screen.move(68,63)
  screen.text("tape")

  screen.update()
end

m.init[pMIX] = function()
  norns.vu = m.mix.vu
  m.mix.in1 = 0
  m.mix.in2 = 0
  m.mix.out1 = 0
  m.mix.out2 = 0
  norns.encoders.set_accel(2,true)
  norns.encoders.set_sens(2,1)
  norns.encoders.set_sens(3,1)
  -- FIXME: remove timer when VU works
  u.time = 0.2
  u.count = -1
  u.callback = function() menu.redraw() end
  u:start()
end

m.deinit[pMIX] = function()
  norns.encoders.set_accel(2,false)
  norns.encoders.set_sens(2,2)
  norns.encoders.set_sens(3,2)
  norns.vu = norns.none
  u:stop()
end

m.mix.vu = function(in1,in2,out1,out2)
  m.mix.in1 = in1
  m.mix.in2 = in2
  m.mix.out1 = out1
  m.mix.out2 = out2
  menu.redraw()
end


-----------------------------------------
-- TAPE

local TAPE_MODE_PLAY = 1
local TAPE_MODE_REC = 2

local TAPE_PLAY_LOAD = 1
local TAPE_PLAY_PLAY = 2
local TAPE_PLAY_STOP = 3
local TAPE_PLAY_PAUSE = 4
local TAPE_REC_ARM = 1
local TAPE_REC_START = 2
local TAPE_REC_STOP = 3

local p_tape_play
local p_tape_rec

m.tape = {}
m.tape.mode = TAPE_MODE_PLAY
m.tape.play = {}
m.tape.play.sel = TAPE_PLAY_LOAD
m.tape.play.status = TAPE_PLAY_STOP
m.tape.play.file = nil
m.tape.play.pos_tick = 0
m.tape.rec = {}
m.tape.rec.file = nil
m.tape.rec.sel = TAPE_REC_ARM
m.tape.rec.pos_tick = 0
m.tape.diskfree = 0

local DISK_RESERVE = 250
local function tape_diskfree()
  if norns.disk then
    m.tape.diskfree = math.floor((norns.disk - DISK_RESERVE) / .192) -- seconds of 48k/16bit stereo disk free with reserve
  end
end


m.key[pTAPE] = function(n,z)
  if n==2 and z==1 then
    m.tape.mode = (m.tape.mode==1) and 2 or 1
    menu.redraw()
  elseif n==3 and z==1 then
    if m.tape.mode == TAPE_MODE_PLAY then
      if m.tape.play.sel == TAPE_PLAY_LOAD then
        local playfile_callback = function(path)
          if path ~= "cancel" then
            audio.tape_play_open(path)
            m.tape.play.file = path:match("[^/]*$")
            m.tape.play.status = TAPE_PLAY_PAUSE
            m.tape.play.sel = TAPE_PLAY_PLAY
            local ch, samples, rate = sound_file_inspect(path)
            m.tape.play.length = math.floor(samples / rate)
            m.tape.play.length_text = util.s_to_hms(m.tape.play.length)
            m.tape.play.pos_tick = 0
            tape_play_counter.time = 0.25
            tape_play_counter.callback = function()
              m.tape.play.pos_tick = m.tape.play.pos_tick + 0.25
              if m.tape.play.pos_tick > m.tape.play.length 
                  and m.tape.play.status == TAPE_PLAY_PLAY then
                print("tape is over!")
                audio.tape_play_stop()
                tape_play_counter:stop()
                m.tape.play.file = nil
                m.tape.play.stats = TAPE_PLAY_STOP
                m.tape.play.sel = TAPE_PLAY_LOAD
              end
              if menu.mode == true and menu.page == pTAPE then
                menu.redraw()
              end
            end
          else
            m.tape.play.file = nil
          end
          menu.redraw()
        end
        fileselect.enter(os.getenv("HOME").."/dust/audio/tape", playfile_callback)
      elseif m.tape.play.sel == TAPE_PLAY_PLAY then
        tape_play_counter:start()
        audio.tape_play_start()
        m.tape.play.status = m.tape.play.sel
        m.tape.play.sel = TAPE_PLAY_STOP
        menu.redraw()
      elseif m.tape.play.sel == TAPE_PLAY_STOP then
        audio.tape_play_stop()
        tape_play_counter:stop()
        m.tape.play.file = nil
        m.tape.play.status = m.tape.play.sel
        m.tape.play.sel = TAPE_PLAY_LOAD
        menu.redraw()
      end
    else -- REC CONTROLS
      if m.tape.rec.sel == TAPE_REC_ARM then
        tape_diskfree()
        m.tape.rec.file = string.format("%04d",norns.state.tape) .. ".wav"
        audio.tape_record_open(audio_dir.."/tape/"..m.tape.rec.file)
        m.tape.rec.sel = TAPE_REC_START
        m.tape.rec.pos_tick = 0
        tape_rec_counter.time = 0.25
        tape_rec_counter.callback = function()
          m.tape.rec.pos_tick = m.tape.rec.pos_tick + 0.25
          if m.tape.rec.pos_tick > m.tape.diskfree then
            print("out of space!")
            audio.tape_record_stop()
            norns.state.tape = norns.state.tape + 1
            tape_rec_counter:stop()
            m.tape.rec.sel = TAPE_REC_ARM
          end
          if menu.mode == true and menu.page == pTAPE then
            menu.redraw()
          end
        end
      elseif m.tape.rec.sel == TAPE_REC_START then
        tape_rec_counter:start()
        audio.tape_record_start()
        m.tape.rec.sel = TAPE_REC_STOP
      elseif m.tape.rec.sel == TAPE_REC_STOP then
        tape_rec_counter:stop()
        norns.state.tape = norns.state.tape + 1
        audio.tape_record_stop()
        m.tape.rec.sel = TAPE_REC_ARM
        tape_diskfree()
      end
      menu.redraw()
    end
  end
end

m.enc[pTAPE] = norns.none

m.redraw[pTAPE] = function()
  screen.clear()

  menu.draw_panel()

  screen.move(128,10)
	screen.level(m.tape.mode==TAPE_MODE_PLAY and 15 or 1)
	screen.text_right("PLAY")
  screen.level(2)
  screen.rect(0.5,13.5,127,2)
  screen.stroke()

  if m.tape.play.file then
    screen.level(2)
    screen.move(0,10)
    screen.text(m.tape.play.file)
    screen.move(0,24)
    screen.text(util.s_to_hms(math.floor(m.tape.play.pos_tick)))
    screen.move(128,24)
    screen.text_right(m.tape.play.length_text)
    screen.level(15)
    screen.move((m.tape.play.pos_tick / m.tape.play.length * 128),13.5)
    screen.line_rel(0,2)
    screen.stroke()
    if m.tape.mode==TAPE_MODE_PLAY then
      screen.level(15)
      screen.move(64,24)
      if m.tape.play.sel == TAPE_PLAY_PLAY then screen.text_center("START")
      elseif m.tape.play.sel == TAPE_PLAY_STOP then screen.text_center("STOP") end
    end
  end

  screen.move(128,48)
  screen.level(m.tape.mode==TAPE_MODE_REC and 15 or 1)
  screen.text_right("REC")
  screen.level(2)
  screen.rect(0.5,51.5,127,2)
  screen.stroke()
  if m.tape.mode==TAPE_MODE_REC then
    screen.level(15)
    screen.move(64,62)
    if m.tape.rec.sel == TAPE_REC_START then screen.text_center("START")
    elseif m.tape.rec.sel == TAPE_REC_STOP then screen.text_center("STOP") end
  end
  if m.tape.rec.sel ~= TAPE_REC_ARM then
    screen.level(1)
    screen.move(0,48)
    screen.text(string.format("%04d",norns.state.tape))
    screen.level(2)
    screen.move(0,62)
    screen.text(util.s_to_hms(math.floor(m.tape.rec.pos_tick)))
  end
  screen.level(2)
  screen.move(127,62)
  screen.text_right(util.s_to_hms(m.tape.diskfree))
  screen.level(15)
  screen.move((m.tape.rec.pos_tick / m.tape.diskfree * 128),51.5)
  screen.line_rel(0,2)
  screen.stroke()

  screen.update()
end

m.init[pTAPE] = function()
  tape_diskfree()
end
m.deinit[pTAPE] = norns.none


