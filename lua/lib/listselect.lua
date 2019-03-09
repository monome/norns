-- listselect utility
-- reroutes redraw/enc/key

local ls = {}

function ls.enter(list, callback)
  ls.list = list
  ls.pos = 0
  ls.callback = callback
  ls.done = false
  ls.selection = nil

  ls.len = tab.count(ls.list)

  if norns.menu.status() == false then
    ls.key_restore = key
    ls.enc_restore = enc
    ls.redraw_restore = redraw
    key = ls.key
    enc = ls.enc
    redraw = norns.none
    norns.menu.init()
  else
    ls.key_restore = norns.menu.get_key()
    ls.enc_restore = norns.menu.get_enc()
    ls.redraw_restore = norns.menu.get_redraw()
    norns.menu.set(ls.enc, ls.key, ls.redraw)
  end
  ls.redraw()
end

function ls.exit()
  if norns.menu.status() == false then
    key = ls.key_restore
    enc = ls.enc_restore
    redraw = ls.redraw_restore
    norns.menu.init()
  else
    norns.menu.set(ls.enc_restore, ls.key_restore, ls.redraw_restore)
  end
  if ls.selection then ls.callback(ls.selection)
  else ls.callback("cancel") end
end


ls.key = function(n,z)
  -- back
  if n==2 and z==1 then
    ls.done = true
  -- select
  elseif n==3 and z==1 then
    ls.selection = ls.list[ls.pos+1]
    ls.done = true
  elseif z == 0 and ls.done == true then
    ls.exit()
  end
end

ls.enc = function(n,d)
  if n==2 then
    ls.pos = util.clamp(ls.pos + d, 0, ls.len - 1)
    ls.redraw()
  end
end


ls.redraw = function()
  local i
  screen.clear()
  screen.move(0,10)
  screen.level(15)
  screen.font_face(1)
  screen.font_size(8)
  for i=1,6 do
    if (i > 2 - ls.pos) and (i < ls.len - ls.pos + 3) then
      screen.move(0,10*i)
      local line = ls.list[i+ls.pos-2]
      if(i==3) then
        screen.level(15)
      else
        screen.level(4)
      end
      --screen.text(string.upper(line))
      screen.text(line)
    end
  end
  screen.update()
end

return ls
