-- menu.lua
-- norns screen-based navigation module

local tab = require 'tabutil'
local util = require 'util'
local fileselect = require 'fileselect'
local listselect = require 'listselect'
local textentry = require 'textentry'

menu = {}

mix = require 'core/mix'

-- global functions for scripts
key = norns.none
enc = norns.none
redraw = norns.blank
cleanup = norns.none

-- tuning
local KEY1_HOLD_TIME = 0.25

menu.mode = false
menu.page = "HOME"
menu.panel = 3
menu.panels = {"MIX", "TAPE", "HOME", "PARAMS"}
menu.alt = false
menu.scripterror = false
menu.locked = true
menu.errormsg = ""
menu.shownav = false
menu.showstats = false
menu.previewfile = ""

-- menu pages
local m = {}

-- METROS
local pending = false
-- metro for key hold detection
local metro = require 'core/metro'
local t = metro[31]
t.time = KEY1_HOLD_TIME
t.count = 1
t.event = function(_)
  menu.key(1,1)
  pending = false
end
-- metro for page status updates
menu.timer = metro[32]
-- metro for nav vanish
local nav_vanish = metro[35]
nav_vanish.time = 1
nav_vanish.event = function()
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
norns.menu.toggle = function(status) menu.set_mode(status) end

norns.scripterror = function(msg)
  if msg == nil then msg = "" end
  print("### SCRIPT ERROR: "..msg)
  menu.errormsg = msg
  menu.scripterror = true
  menu.locked = true
  menu.set_page("HOME")
  menu.set_mode(true)
end

norns.init_done = function(status)
  menu.set_page("HOME")
  menu.panel = 3
  if status == true then
    menu.scripterror = false
    m["PARAMS"].pos = 0
    menu.locked = false
    menu.set_mode(false)
  end
  m["PARAMS"].init_map()
  m["PARAMS"].read_pmap()
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
  screen.ping()
end

-- menu set mode
menu.set_mode = function(mode)
  if mode == false then -- PLAY MODE
    if menu.mode == true then _norns.screen_restore() end
    menu.mode = false
    m[menu.page].deinit()
    redraw = norns.script.redraw
    menu.key = key
    norns.encoders.callback = enc
    norns.encoders.set_accel(0,false)
    norns.encoders.set_sens(0,1)
    redraw()
  else -- MENU MODE
    if menu.mode == false then _norns.screen_save() end
    menu.mode = true
    menu.alt = false
    redraw = norns.none
    screen.font_face(1)
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
  m[menu.page].deinit()
  menu.page = page
  menu.key = m[page].key
  menu.penc = m[page].enc
  menu.redraw = m[page].redraw
  m[page].init()
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




-- interfaces


m["HOME"] = require 'core/menu/home'
m["SELECT"] = require 'core/menu/select'
m["PREVIEW"] = require 'core/menu/preview'
m["PARAMS"] = require 'core/menu/params'
m["SYSTEM"] = require 'core/menu/system'
m["DEVICES"] = require 'core/menu/devices'
m["WIFI"] = require 'core/menu/wifi'
m["AUDIO"] = require 'core/menu/audio'
m["RESET"] = require 'core/menu/reset'
m["UPDATE"] = require 'core/menu/update'
m["SLEEP"] = require 'core/menu/sleep'
m["MIX"] = require 'core/menu/mix'
m["TAPE"] = require 'core/menu/tape'


