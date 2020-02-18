local m = {
  pos = 1,
  list = {"DEVICES > ", "WIFI >", "RESET", "UPDATE"},
  pages = {"DEVICES", "WIFI", "RESET", "UPDATE"}
}

m.key = function(n,z)
  if n==2 and z==1 then
    _menu.set_page("HOME")
  elseif n==3 and z==1 then
    _menu.set_page(m.pages[m.pos])
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = util.clamp(m.pos + delta, 1, #m.list)
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()

  for i=1,#m.list do
    screen.move(0,10+10*i)
    if(i==m.pos) then
      screen.level(15)
    else
      screen.level(4)
    end
    screen.text(m.list[i])
  end

  screen.update()
end

m.init = norns.none
m.deinit = norns.none

return m
