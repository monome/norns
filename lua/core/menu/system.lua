local m = {
  pos = 1,
  list = {"DEVICES > ", "WIFI >", "MODS >", "KBD LAYOUT >", "RESTART", "RESET", "UPDATE"},
  pages = {"DEVICES", "WIFI", "MODS", "KBD LAYOUT", "RESTART", "RESET", "UPDATE"}
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

  for i=1,6 do
    if (i > 2 - m.pos - 1) and (i < #m.list - m.pos + 4) then
      local name = m.list[i+m.pos-1-2]

      local y = 10*i
      local line_level = 4
      if i==3 then
        line_level = 15
      end
      screen.level(line_level)

      screen.move(2,y)
      if name then
        screen.text(name)
      end
    end
  end

  screen.update()
end

m.init = norns.none
m.deinit = norns.none

return m
