-- menu.lua
-- norns screen-based navigation module

local tab = require 'tabutil'
local util = require 'util'
local paramset = require 'paramset'
local fx = require 'effects'
local cs = require 'controlspec'
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
cs.MAIN_LEVEL = cs.new(-60,0,'lin',0,0,"dB")
mix:add_control("output",cs.MAIN_LEVEL)
mix:set_action("output",
  function(x) norns.audio.output_level(x) end)
mix:add_control("input",cs.MAIN_LEVEL)
mix:set_action("input",
  function(x)
    norns.audio.input_level(1,x)
    norns.audio.input_level(2,x)
  end)
cs.MUTE_LEVEL = cs.new(-60,0,'lin',0,-60,"dB")
mix:add_control("monitor",cs.MUTE_LEVEL)
mix:set_action("monitor",
  function(x) norns.audio.monitor_level(x) end)
mix:add_option("monitor mode",{"STEREO","MONO"})
mix:set_action("monitor mode",
  function(x)
    if x == 1 then norns.audio.monitor_stereo()
    else norns.audio.monitor_mono() end
  end)
mix:add_number("headphone",0, 63, 40)
mix:set_action("headphone",
  function(x) gain_hp(x) end) 

-- TODO TAPE (rec) modes: OUTPUT, OUTPUT+MONITOR, OUTPUT/MONITOR SPLIT
-- TODO TAPE (playback) VOL, SPEED?
  
-- ControlSpec.new(minval, maxval, warp, step, default, units) 
mix:add_separator()
mix:add_option("aux fx", {"OFF","ON"}, 2)
mix:set_action("aux fx",
  function(x)
    if x == 1 then
      fx.aux_fx_off()
    else
      fx.aux_fx_on()
    end
  end)
cs.DB_LEVEL = cs.new(-60,18,'lin',0,0,"dB")
cs.DB_LEVEL_MUTE = cs.new(-60,18,'lin',0,-60,"dB")
cs.DB_LEVEL_9DB = cs.new(-60,18,'lin',0,-9,"dB")
mix:add_control("aux output level", cs.DB_LEVEL_9DB)
mix:set_action("aux output level",
  function(x) fx.aux_fx_output_level(x) end) 
mix:add_control("aux input1 level", cs.DB_LEVEL_MUTE)
mix:set_action("aux input1 level",
  function(x) fx.aux_fx_input_level(1,x) end) 
mix:add_control("aux input2 level", cs.DB_LEVEL_MUTE)
mix:set_action("aux input2 level",
  function(x) fx.aux_fx_input_level(2,x) end) 
mix:add_control("aux input1 pan", cs.PAN)
mix:set_action("aux input1 pan",
  function(x) fx.aux_fx_input_pan(1,x) end) 
mix:add_control("aux input2 pan", cs.PAN)
mix:set_action("aux input2 pan",
  function(x) fx.aux_fx_input_pan(2,x) end) 
mix:add_control("aux return level", cs.DB_LEVEL)
mix:set_action("aux return level",
  function(x) fx.aux_fx_return_level(x) end) 


cs.IN_DELAY = cs.new(20,100,'lin',0,60,'ms')
mix:add_control("rev in delay", cs.IN_DELAY)
mix:set_action("rev in delay",
  function(x) fx.aux_fx_param("rev_in_delay",x) end)

cs.LF_X = cs.new(50,1000,'exp',0,200,'hz')
mix:add_control("rev lf x", cs.LF_X)
mix:set_action("rev lf x",
  function(x) fx.aux_fx_param("rev_lf_x",x) end)

cs.RT60 = cs.new(1,8,'lin',0,3,'s')
mix:add_control("rev low rt60", cs.RT60)
mix:set_action("rev low rt60",
  function(x) fx.aux_fx_param("low_rt60",x) end) 
mix:add_control("rev mid rt60", cs.RT60)
mix:set_action("rev mid rt60",
  function(x) fx.aux_fx_param("mid_rt60",x) end)

cs.HF_DAMP = cs.new(1500,20000,'exp',0,6000,'hz')
mix:add_control("rev hf damping", cs.HF_DAMP)
mix:set_action("rev hf damping",
  function(x) fx.aux_fx_param("hf_damping",x) end) 

--[[
cs.EQ_FREQ1 = cs.new(40,2500,'exp',0,315,'hz')
mix:add_control("rev eq1 freq", cs.EQ_FREQ1)
mix:set_action("rev eq1 freq",
  function(x) fx.aux_fx_param("eq1_freq",x) end)
cs.EQ_LVL = cs.new(-15,15,'lin',0,0,"dB")
mix:add_control("rev eq1 level", cs.EQ_LVL)
mix:set_action("rev eq1 level",
  function(x) fx.aux_fx_param("eq1_level",x) end)

cs.EQ_FREQ2 = cs.new(160,10000,'exp',0,1500,'hz')
mix:add_control("rev eq2 freq", cs.EQ_FREQ2)
mix:set_action("rev eq2 freq",
  function(x) fx.aux_fx_param("eq2_freq",x) end)
mix:add_control("rev eq2 level", cs.EQ_LVL)
mix:set_action("rev eq2 level",
  function(x) fx.aux_fx_param("eq2_level",x) end)
--]]

--cs.LEVEL = cs.new(-70,40,'lin',0,0,'dB')
--mix:add_control("rev_level", cs.LEVEL)
--mix:set_action("rev_level",
  --function(x) fx.aux_fx_param("level",x) end)


mix:add_separator()
mix:add_option("insert fx", {"OFF","ON"})
mix:set_action("insert fx",
  function(x)
    if x == 1 then
      fx.insert_fx_off()
    else
      fx.insert_fx_on()
    end
  end)
cs.MIX = cs.new(0,1,'lin',0,1,'')
mix:add_control("insert mix", cs.MIX)
mix:set_action("insert mix",
  function(x) fx.insert_fx_mix(x) end) 

cs.RATIO = cs.new(1,20,'lin',0,4,'')
mix:add_control("comp ratio", cs.RATIO)
mix:set_action("comp ratio",
  function(x) fx.insert_fx_param("level",x) end)

cs.THRESH = cs.new(-100,10,'lin',0,-18,'dB')
mix:add_control("comp threshold", cs.THRESH)
mix:set_action("comp threshold",
  function(x) fx.insert_fx_param("threshold",x) end)

cs.ATTACK = cs.new(1,1000,'exp',0,5,'ms')
mix:add_control("comp attack", cs.ATTACK)
mix:set_action("comp attack",
  function(x) fx.insert_fx_param("attack",x) end) 
cs.RELEASE = cs.new(1,1000,'exp',0,50,'ms')
mix:add_control("comp release", cs.RELEASE)
mix:set_action("comp release",
  function(x) fx.insert_fx_param("release",x) end)

cs.MAKEUP = cs.new(-60,60,'lin',0,9,'dB')
mix:add_control("comp makeup gain", cs.MAKEUP)
mix:set_action("comp makeup gain",
  function(x) fx.insert_fx_param("makeup_gain",x) end) 




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
local paPLAY = 0
local paPAUSE = 1
local paSTOP = 2

local tape = {}
tape.key = false
tape.mode = tOFF
tape.lastmode = tREC
tape.selectmode = false
tape.playaction = paPLAY
tape.status = 0
tape.name = ""
tape.time = 0
tape.playfile = ""

m.key[pMIX] = function(n,z)
  if n==3 and z==1 and tape.key == false then
    menu.set_page(pHOME)
  elseif n==2 then
    if z==1 then
      tape.key = true
      if tape.mode == tOFF then tape.selectmode = true end
    else
      tape.key = false
      tape.selectmode = false
    end
  elseif n==3 and tape.key == true and z==1 then
    if tape.mode == tOFF then
      if tape.lastmode == tREC then
        -- REC: ready
        tape.selectmode = false
        tape.name = os.date("%y-%m-%d_%H-%M") .. ".aif"
        tape_new(tape.name)
        print("new tape > "..tape.name)
        tape.mode = tREC
        tape.status = tsPAUSE
        redraw()
      else
        -- PLAY: select tape
        local playfile_callback = function(path)
          if path ~= "cancel" then
            tape.playfile = path
            tape.playfile = tape.playfile:match("[^/]*$") -- strip path from name
            tape_open(tape.playfile)
            tape.mode = tPLAY
            tape.status = tsPAUSE
            tape.playaction = paPLAY
            tape.metro = metro.alloc()
            tape.metro.callback = function() tape.time = tape.time + 1 end
            tape.metro.count = -1
            tape.metro.time = 1
            tape.time = 0
          else
            tape.mode = tOFF
          end
          tape.key = false
          tape.selectmode = false
          redraw()
        end
        fileselect.enter(os.getenv("HOME").."/dust/audio/tape", playfile_callback)
      end
    elseif tape.mode == tREC and tape.status == tsPAUSE then
      -- REC: start recording
      tape.status = tsREC
      tape_start_rec()
      tape.metro = metro.alloc()
      tape.metro.callback = function() tape.time = tape.time + 1 end
      tape.metro.count = -1
      tape.metro.time = 1
      tape.time = 0
      tape.metro:start()
    elseif tape.mode == tREC and tape.status == tsREC then
      -- REC: stop recording
      print("stopping tape")
      tape_stop_rec()
      tape.mode = tOFF
      tape.metro:stop()
    elseif tape.mode == tPLAY and tape.playaction == paPLAY then
      tape_play()
      tape.metro:start()
      tape.status = paPLAY
      tape.playaction = paPAUSE
    elseif tape.mode == tPLAY and tape.playaction == paPAUSE then
      tape_pause()
      tape.metro:stop()
      tape.status = paPAUSE
      tape.playaction = paPLAY
    elseif tape.mode == tPLAY and tape.playaction == paSTOP then
      tape_stop()
      tape.metro:stop()
      tape.mode = tOFF
    end
  end
end

m.enc[pMIX] = function(n,d)
  if n==2 then
    if tape.key == false then
      mix:delta("input",d)
    end
  elseif n==3 then
    if tape.key == false then
      mix:delta("monitor",d)
    elseif tape.selectmode == true then
      if d < 0 then tape.lastmode = tREC
      else tape.lastmode = tPLAY end
    elseif tape.mode == tPLAY then
      if d < 0 then tape.playaction = paSTOP
      elseif tape.status == paPLAY then tape.playaction = paPAUSE
      elseif tape.status == paPAUSE then tape.playaction = paPLAY
      end
    end
  end
end

m.redraw[pMIX] = function()
  local n
  screen.clear()
  screen.aa(1)
  screen.line_width(1)

  local x = -40
  screen.level(2)
  n = (60+mix:get("output"))/60*48
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
  n = (60+mix:get("input"))/60*48
  screen.rect(x+64,56,2,-n)
  screen.stroke()

  screen.level(15)
  n = m.mix.in1/64*48
  screen.rect(x+70,56,2,-n)
  screen.stroke()
  n = m.mix.in2/64*48
  screen.rect(x+76,56,2,-n)
  screen.stroke()

  screen.level(2)
  n = (60+mix:get("monitor"))/60*48
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

  if tape.selectmode then
    screen.level(10)
    screen.move(90,40)
    if tape.lastmode == tPLAY then
      screen.text_center("TAPE > PLAY")
    else
      screen.text_center("TAPE > REC")
    end
  end

  if tape.mode == tREC then
    screen.move(90,40)
    if tape.status == tsPAUSE then
      screen.text_center("READY")
    elseif tape.status == tsREC then
      screen.text_center("RECORDING")
      screen.move(90,48)
      local min = math.floor(tape.time / 60)
      local sec = tape.time % 60
      screen.text_center(min..":"..sec)
    end
  elseif tape.mode == tPLAY then
    screen.move(90,40)
    if tape.key then
      if tape.playaction == paPAUSE then
        screen.text_center("TAPE > PAUSE")
      elseif tape.playaction == paPLAY then
        screen.text_center("TAPE > PLAY")
      elseif tape.playaction == paSTOP then
        screen.text_center("TAPE > STOP")
      end
    else
      screen.text_center("TAPE")
    end
    screen.level(4)
    screen.move(90,48)
    local min = math.floor(tape.time / 60)
    local sec = tape.time % 60
    screen.text_center(min..":"..sec)
    screen.move(90,62)
    screen.text_center(tape.playfile)
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
m.sys.pages = {pAUDIO, pWIFI, pSYNC, pUPDATE, pLOG}
m.sys.input = 0
m.sys.disk = ""

m.key[pSYSTEM] = function(n,z)
  if n==2 and z==1 then
    norns.state.save()
    menu.set_page(pHOME)
  elseif n==3 and z==1 then
    menu.set_page(m.sys.pages[m.sys.pos+1])
  end
end

m.enc[pSYSTEM] = function(n,delta)
  if n==2 then
    m.sys.pos = m.sys.pos + delta
    m.sys.pos = util.clamp(m.sys.pos, 0, #m.sys.list - 1)
    menu.redraw()
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
  screen.text_right(norns.version.update)
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
-- SYNC
m.sync = {}
m.sync.pos = 0

m.key[pSYNC] = function(n,z)
  if n==2 and z==1 then
    menu.set_page(pSYSTEM)
  elseif n==3 and z==1 and m.sync.disk=='' then
    menu.set_page(pSYSTEM)
  elseif n==3 and z==1 and m.sync.pos==0 then
    m.sync.busy = true
    menu.redraw()
    os.execute("sudo rsync --recursive --links --verbose --update $HOME/dust/ "..m.sync.disk.."/dust; sudo sync")
    norns.log.post("sync to usb")
    menu.set_page(pSYSTEM)
  elseif n==3 and z==1 and m.sync.pos==1 then
    m.sync.busy = true
    menu.redraw()
    os.execute("rsync --recursive --links --verbose --update "..m.sync.disk.."/dust/ $HOME/dust; sudo sync")
    norns.log.post("sync from usb")
    menu.set_page(pSYSTEM)
  elseif n==3 and z==1 and m.sync.pos==2 then
    os.execute("sudo umount "..m.sync.disk)
    norns.log.post("usb disk ejected")
    menu.set_page(pSYSTEM)
  end
end

m.enc[pSYNC] = function(n,delta)
  if n==2 then
    m.sync.pos = util.clamp(m.sync.pos+delta, 0, 2)
    menu.redraw()
  end
end

m.redraw[pSYNC] = function()
  screen.clear()
  screen.level(10)
  if m.sync.disk=='' then
    screen.move(0,30)
    screen.text("no usb disk available")
  elseif m.sync.busy then
    screen.move(0,30)
    screen.text("usb sync... (wait)")
  else
    screen.level(m.sync.pos==0 and 10 or 3)
    screen.move(0,40)
    screen.text("SYNC TO USB") 
    screen.level(m.sync.pos==1 and 10 or 3)
    screen.move(0,50)
    screen.text("SYNC FROM USB") 
    screen.level(m.sync.pos==2 and 10 or 3)
    screen.move(0,60)
    screen.text("EJECT USB") 
  end
  screen.update()
end

m.init[pSYNC] = function()
  m.sync.pos = 0
  m.sync.busy = false
  m.sync.disk = util.os_capture("lsblk -o mountpoint | grep media")
end

m.deinit[pSYNC] = function()
end


-----------------------------------------
-- UPDATE
m.update = {}
m.update.pos = 0
m.update.confirm = false

m.key[pUPDATE] = function(n,z)
  if n==2 and z==1 then
    menu.set_page(pSYSTEM)
  elseif n==3 and z==1 and #m.update.list == 0 then
    menu.set_page(pSYSTEM)
  elseif n==3 and z==1 and m.update.confirm == false and #m.update.list > 0 then
    -- CONFIRM UPDATE
    m.update.confirm = true
    menu.redraw()
  elseif n==3 and z==1 and m.update.confirm == true then
    -- RUN UPDATE
    m.update.busy = true
    menu.redraw()
    os.execute("$HOME/update/"..m.update.list[m.update.pos+1].."/update.sh")
    menu.set_page(pHOME)
  end
end

m.enc[pUPDATE] = function(n,delta)
  if n==2 then
    m.update.pos = util.clamp(m.update.pos+delta, 0, #m.update.list - 1) --4 = options
    menu.redraw()
  end
end

m.redraw[pUPDATE] = function()
  screen.clear()
  screen.level(15)
  if m.update.checking then
    screen.move(64,32)
    screen.text_center("checking for updates")
  elseif m.update.busy then
    screen.move(64,32)
    screen.text_center("updating... (wait)")
  elseif #m.update.list == 0 then
    screen.move(64,32)
    screen.text_center("no updates")
  elseif m.update.confirm == false then
    for i=1,6 do
      if (i > 2 - m.update.pos) and (i < #m.update.list - m.update.pos + 3) then
        screen.move(0,10*i)
        line = m.update.list[i+m.update.pos-2]
        if(i==3) then
          screen.level(15)
        else
          screen.level(4)
        end
        screen.text(line)
      end
    end
  else
    screen.move(64,32)
    screen.text_center("run update? "..m.update.list[m.update.pos+1])
  end
  screen.update()
end

m.init[pUPDATE] = function()
  m.update.confirm = false
  m.update.pos = 0
  local i, t, popen = 0, {}, io.popen
  m.update.checking = true
  m.update.busy = false
  menu.redraw()
  -- CHECK FOR UPDATE FOLDER
  local test = util.os_capture("ls $HOME | grep update")
  if test ~= "update" then os.execute("mkdir $HOME/update") end
  -- COPY FROM USB
  local disk = util.os_capture("lsblk -o mountpoint | grep media")
  local pfile = popen("ls -p "..disk.."/norns*.tgz")
  for filename in pfile:lines() do
    os.execute("cp "..filename.." $HOME/update/")
  end 
  -- PREPARE
  pfile = popen('ls -p $HOME/update/norns*.tgz')
  for filename in pfile:lines() do
    print(filename)
    -- extract
    os.execute("tar -xzvC $HOME/update -f "..filename)
    -- check md5
    local md5 = util.os_capture("cd $HOME/update; md5sum -c *.md5")
    print(">> "..md5)
    if string.find(md5,"OK") then
      norns.log.post("new update found")
      -- unpack
      local file = string.sub(md5,1,-5)
      os.execute("tar -xzvC $HOME/update -f $HOME/update/"..file)
    else norns.log.post("bad update file") end
    -- delete
    os.execute("rm $HOME/update/*.tgz; rm $HOME/update/*.md5")
  end
  pfile:close()
  m.update.list = {}
  print("LIST")
  pfile = popen('ls -tp $HOME/update')
  for file in pfile:lines() do
    table.insert(m.update.list,string.sub(file,0,-2))
  end
  tab.print(m.update.list)
  m.update.checking = false
  menu.redraw()
end

m.deinit[pUPDATE] = function()
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
    m.sleep = true
    menu.redraw()
    norns.audio.output_level(-100)
    gain_hp(0)
    --norns.audio.set_audio_level(0)
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


