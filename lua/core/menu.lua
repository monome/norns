-- menu.lua
-- norns screen-based navigation module

local tab = require 'tabutil'
local util = require 'util'
local fileselect = require 'fileselect'
local listselect = require 'listselect'
local textentry = require 'textentry'

_menu = {}

-- global functions for scripts
key = norns.none
enc = norns.none
redraw = norns.blank
cleanup = norns.none

-- tuning
local KEY1_HOLD_TIME = 0.25

_menu.mode = false
_menu.page = "HOME"
_menu.panel = 3
_menu.panels = {"MIX", "TAPE", "HOME", "PARAMS"}
_menu.alt = false
_menu.scripterror = false
_menu.locked = true
_menu.errormsg = ""
_menu.shownav = false
_menu.showstats = false
_menu.previewfile = ""

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
  _menu.key(1,1)
  pending = false
end
-- metro for page status updates
_menu.timer = metro[32]
-- metro for nav vanish
local nav_vanish = metro[35]
nav_vanish.time = 1
nav_vanish.event = function()
  _menu.shownav = false
  if _menu.mode == true then _menu.redraw() end
  nav_vanish:stop()
end
-- screen.lua has metro[35] for screensaver

-- FIXME: get rid of norns.menu -- just put stuff in _menu
-- assigns key/enc/screen handlers after user script has loaded
norns.menu = {}
norns.menu.init = function() _menu.set_mode(_menu.mode) end -- used by fileselect.lua
norns.menu.status = function() return _menu.mode end
norns.menu.set = function(new_enc, new_key, new_redraw)
  _menu.penc = new_enc
  _menu.key = new_key
  _menu.redraw = new_redraw
end
norns.menu.get_enc = function() return _menu.penc end
norns.menu.get_key = function() return _menu.key end
norns.menu.get_redraw = function() return _menu.redraw end
norns.menu.toggle = function(status) menu.set_mode(status) end

norns.scripterror = function(msg)
  if msg == nil then msg = "" end
  print("### SCRIPT ERROR: "..msg)
  _menu.errormsg = msg
  _menu.scripterror = true
  _menu.locked = true
  _menu.set_page("HOME")
  _menu.set_mode(true)
end

norns.init_done = function(status)
  _menu.set_page("HOME")
  _menu.panel = 3
  if status == true then
    _menu.scripterror = false
    m["PARAMS"].pos = 0
    _menu.locked = false
    _menu.set_mode(false)
  end
  m["PARAMS"].init_map()
  m["PARAMS"].read_pmap()
end




-- input redirection

_menu.enc = function(n, delta)
  if n==1 and _menu.alt == false then
    --mix:delta("output",delta)
    local c = util.clamp(_menu.panel+delta,1,4)
    if c ~= _menu.panel then
      _menu.shownav = true
      _menu.panel = c
      _menu.set_page(_menu.panels[_menu.panel])
      nav_vanish:start()
    end
  else _menu.penc(n, delta) end
end


_norns.key = function(n, z)
  -- key 1 detect for short press
  if n == 1 then
    if z == 1 then
      _menu.alt = true
      pending = true
      t:start()
    elseif z == 0 and pending == true then
      _menu.alt = false
      if _menu.mode == true and _menu.locked == false then
        _menu.set_mode(false)
      else _menu.set_mode(true) end
      t:stop()
      pending = false
    elseif z == 0 then
      _menu.alt = false
      _menu.key(n,z) -- always 1,0
      if _menu.mode == true then _menu.redraw() end
    else
      _menu.key(n,z) -- always 1,1
    end
    -- key 2/3 pass
  else
    _menu.key(n,z)
  end
  screen.ping()
end

-- _menu.set mode
_menu.set_mode = function(mode)
  if mode == false then -- PLAY MODE
    if _menu.mode == true then _norns.screen_restore() end
    _menu.mode = false
    m[_menu.page].deinit()
    screen.clear()
    screen.update()
    redraw = norns.script.redraw
    _menu.key = key
    norns.encoders.callback = enc
    norns.enc.resume()
    --norns.encoders.set_accel(0,false)
    --norns.encoders.set_sens(0,1)
    redraw()
  else -- _menu.MODE
    if _menu.mode == false then _norns.screen_save() end
    _menu.mode = true
    _menu.alt = false
    redraw = norns.none
    screen.font_face(1)
    screen.font_size(8)
    screen.line_width(1)
    norns.encoders.callback = _menu.enc
    norns.encoders.set_accel(1,false)
    norns.encoders.set_sens(1,8)
    norns.encoders.set_accel(2,false)
    norns.encoders.set_sens(2,2)
    norns.encoders.set_accel(3,true)
    norns.encoders.set_sens(3,2)
    _menu.set_page(_menu.page)
  end
end

-- set page
_menu.set_page = function(page)
  m[_menu.page].deinit()
  _menu.page = page
  _menu.key = m[page].key
  _menu.penc = m[page].enc
  _menu.redraw = m[page].redraw
  m[page].init()
  _menu.redraw()
end

-- draw panel indicator
function _menu.draw_panel()
  if _menu.shownav then
    screen.aa(1)
    screen.line_width(1)
    for i = 1,4 do
      screen.level(i == _menu.panel and 8 or 2)
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

