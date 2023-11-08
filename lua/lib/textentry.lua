--- textentry UI
-- @module lib.textentry

local te_kbd_cb = require 'lib/textentry_kbd'
local keyboard = require 'core/keyboard'

local te = {}

local function keycode(c,value)
  if keyboard.state.ESC then
    te.txt = nil
    te.exit()
  elseif keyboard.state.ENTER then
    te.exit()
  elseif keyboard.state.BACKSPACE then
    te.row = 1
    te.delok = 0
    te.txt = string.sub(te.txt,0,-2)
    if te.check then
      te.warn = te.check(te.txt)
    end
    te.redraw()
  end
end

local function keychar(a)
  te.row = 0
  te.pos = string.byte(a) - 5 - 32
  te.txt = te.txt .. a
  if te.check then
    te.warn = te.check(te.txt)
  end
  te.redraw()
end

te.enter = function(callback, default, heading, check)
  te.txt = default or ""
  te.heading = heading or ""
  te.pos = 28
  if default then te.row=1 else te.row = 0 end
  te.delok = 1
  te.callback = callback
  te.check = check
  te.warn = nil
  te.pending = false
  te_kbd_cb.code = keycode
  te_kbd_cb.char = keychar

  if norns.menu.status() == false then
    te.key_restore = key
    te.enc_restore = enc
    te.redraw_restore = redraw
    te.refresh_restore = refresh
    key = te.key
    enc = te.enc
    norns.menu.init()
    redraw = norns.none
  else
    te.key_restore = norns.menu.get_key()
    te.enc_restore = norns.menu.get_enc()
    te.redraw_restore = norns.menu.get_redraw()
    te.refresh_restore = norns.menu.get_refresh()
    norns.menu.set(te.enc, te.key, te.redraw, te.refresh)
  end
  te.redraw()
end

te.exit = function()
  te_kbd_cb.code = nil
  te_kbd_cb.char = nil

  if norns.menu.status() == false then
    key = te.key_restore
    enc = te.enc_restore
    redraw = te.redraw_restore
    refresh = te.refresh_restore
    norns.menu.init()
  else
    norns.menu.set(te.enc_restore, te.key_restore, te.redraw_restore, te.refresh_restore)
  end
  if te.txt then te.callback(te.txt)
  else te.callback(nil) end
end


te.key = function(n,z)
  if n==2 and z==0 then
    te.txt = nil
    te.exit()
  elseif n==3 and z==1 then
    if te.row == 0 then
      local ch = ((5+te.pos)%95)+32
      te.txt = te.txt .. utf8.char(ch)
      if te.check then
        te.warn = te.check(te.txt)
      end
      te.redraw()
    else
      if te.delok==0 then
        te.txt = string.sub(te.txt,0,-2)
        if te.check then
          te.warn = te.check(te.txt)
	end
        te.redraw()
      elseif te.delok == 1 then
        te.pending = true
      end
    end
  elseif n==3 and z==0 and te.pending == true then
    if te.row == 1 and te.delok==1 then te.exit() end
  end
end

te.enc = function(n,delta)
  if n==2 then
    if te.row == 1 then
      if delta > 0 then te.delok = 1
      else te.delok = 0 end
    else te.pos = (te.pos + delta) % 95 end
    te.redraw()
  elseif n==3 then
    if delta > 0 then te.row = 1
    else te.row = 0 end
    te.redraw()
  end
end

te.redraw = function()
  screen.clear()
  screen.level(15)
  screen.move(0,16)
  screen.text(te.heading)
  screen.move(0,32)
  screen.text(te.txt)
  if te.warn ~= nil then
    screen.move(128,32)
    screen.text_right(te.warn)
  end
  for x=0,15 do
    if x==5 and te.row==0 then screen.level(15) else screen.level(2) end
    screen.move(x*8,46)
    screen.text(utf8.char((x+te.pos)%95+32))
  end

  screen.move(0,60)
  if te.row==1 and te.delok==0 then screen.level(15) else screen.level(2) end
  screen.text("DEL")
  screen.move(127,60)
  if te.row==1 and te.delok==1 then screen.level(15) else screen.level(2) end
  screen.text_right("OK")

  screen.update()
end

te.refresh = function() te.redraw() end

return te
