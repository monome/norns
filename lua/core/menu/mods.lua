local mods = require 'core/mods'

local m = {
  pos = 0,
  list = {},
  selected = ""
}

m.key = function(n,z)
  -- back
  if n==2 and z==1 then
    _menu.set_page("SYSTEM")
  elseif n==3 and z==1 and m.len > 0 then -- if there are mods
    -- TODO: check if mod is enabled first!
    if _menu.m[m.selected] then -- does the mod have a menu page
      _menu.set_page(m.selected)
    end
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = util.clamp(m.pos + delta, 0, m.len - 1)
    m.selected = string.upper(m.list[m.pos+1])
    _menu.redraw()
  elseif n==3 then
    -- TODO
    if d > 0 then
      -- ENABLE MOD
    else
      -- DISABLE MOD
    end
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
        local line = string.upper(m.list[i+m.pos-2])
        if(i==3) then
          screen.level(15)
        else
          screen.level(4)
        end
        if _menu.m[line] then line = line .. " >" end
        screen.text(line)
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

