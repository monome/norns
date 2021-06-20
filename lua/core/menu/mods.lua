local mods = require 'core/mods'

local m = {
  pos = 0,
  list = {},
}

m.key = function(n,z)
  -- back
  if n==2 and z==1 then
    _menu.set_page("SYSTEM")
  else
    print("do something", n, z)
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = util.clamp(m.pos + delta, 0, m.len - 1)
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()
  screen.level(15)
  if m.len == 0 then
    screen.move(64,40)
    screen.text_center("no mods")
  else
    for i=1,6 do
      if (i > 2 - m.pos) and (i < m.len - m.pos + 3) then
        screen.move(0,10*i)
        local line = m.list[i+m.pos-2]
        if(i==3) then
          screen.level(15)
        else
          screen.level(4)
        end
        screen.text(string.upper(line))
      end
    end
  end
  screen.update()
end

m.init = function()
  m.list = {}
  for k, _ in pairs(mods.scan() or {}) do
    table.insert(m.list, k)
  end
  m.len = tab.count(m.list)
end

m.deinit = function() end

return m