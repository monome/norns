--- te class

local te = {}

te.enter = function(callback, default)
  te.txt = default or ""
  te.pos = 27
  te.row = 0
  te.delok = 1
  te.callback = callback

  te.key_restore = key
  te.enc_restore = enc
  te.redraw_restore = redraw
  key = te.key
  enc = te.enc
  redraw = te.redraw
  norns.menu.init() 
end

te.exit = function()
  key = te.key_restore
  enc = te.enc_restore
  redraw = te.redraw_restore 
  norns.menu.init()
  if te.txt then te.callback(te.txt)
  else te.callback(nil) end
end


te.key = function(n,z)
  if n==2 and z==1 then
    te.txt = nil
    te.exit()
  elseif n==3 and z==1 then
    if te.row == 0 then
      local ch = ((5+te.pos)%94)+33
      te.txt = te.txt .. string.char(ch)
      redraw()
    else
      if te.delok==0 then
        te.txt = string.sub(te.txt,0,-2)
      elseif te.delok==1 then
        te.exit()
      end
      redraw()
    end
  end
end

te.enc = function(n,delta)
  if n==2 then
    if te.row == 1 then
      if delta > 0 then te.delok = 1
      else te.delok = 0 end
    else te.pos = (te.pos + delta) % 94 end
    redraw()
  elseif n==3 then
    if delta > 0 then te.row = 1
    else te.row = 0 end
    redraw()
  end
end

te.redraw = function()
  screen.clear()
  screen.level(15)
  screen.move(0,32)
  screen.text(te.txt)
  local x,y
  for x=0,15 do
    if x==5 and te.row==0 then screen.level(15) else screen.level(2) end
    screen.move(x*8,46)
    screen.text(string.char((x+te.pos)%94+33))
  end

  screen.move(0,60)
  if te.row==1 and te.delok==0 then screen.level(15) else screen.level(2) end
  screen.text("DEL")
  screen.move(127,60)
  if te.row==1 and te.delok==1 then screen.level(15) else screen.level(2) end
  screen.text_right("OK")

  screen.update()
end

return te
