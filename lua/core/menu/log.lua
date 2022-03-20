local m = {}

local s = 500
local go = false

m.init = function()
  go = false
end

m.key = function(n,z)
  if n==3 and z==1 then
    print("export logs!")
    os.execute("journalctl -n "..s.." > ~/dust/data/system.log")
    _menu.set_page("SYSTEM")
  elseif n==2 and z==1 then
    _menu.set_page("SYSTEM")
  end
end

m.enc = function(n,d)
  if (n==3) then
    s = util.clamp(s + d*10,100,10000)
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()
  screen.move(64,40)
  screen.level(15)
  if done then
    screen.text_center("logs exported!")
  else
    screen.text_center("logs size:")
    screen.move(64,48)
    screen.text_center(s)
  end
  screen.update()
end

m.deinit = norns.none

return m
