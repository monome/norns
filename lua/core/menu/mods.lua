local mods = require 'core/mods'
local tabutil = require 'tabutil'

local m = {
  pos = 0,
  list = {},
  selected = ""
}

m.position_name = function(pos)
  return m.list[pos+1]
end

m.select_position = function(pos)
  m.selected = m.position_name(pos)
end

m.key = function(n,z)
  -- back
  if n==2 and z==1 then
    _menu.set_page("SYSTEM")
  elseif n==3 and z==1 and m.len > 0 then -- if there are mods
    if _menu.m[m.selected] then -- does the mod have a menu page
      _menu.set_page(m.selected)
    end
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = util.clamp(m.pos + delta, 0, m.len - 1)
    m.select_position(m.pos)
    _menu.redraw()
  elseif n==3 then
    if delta > 0 then
      -- print("enable", m.selected)
      m.set_enabled(true)
    else
      -- print("disable", m.selected)
      m.set_enabled(false)
    end
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
        local name = m.list[i+m.pos-2]
        local enabled = mods.is_enabled(name)
        local loaded = mods.is_loaded(name)
        local line = string.upper(name)

        local y = 10*i
        local line_level = 4
        if i==3 then
          line_level = 15
        end
        screen.level(line_level)

        -- loaded indicator
        if loaded then
          screen.move(0,y-2)
          screen.text(".")
        end

        -- selected item
        if _menu.m[name] then line = line .. " >" end
        screen.move(4,y)
        screen.text(line)

        -- change enabled state indicator
        if loaded ~= enabled then
          screen.move(120,y)
          if enabled then
            screen.text("+")
          else
            screen.text("-")
          end
        end
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
  table.sort(m.list)

  m.len = tab.count(m.list)
  if m.len > 0 then
    m.select_position(m.pos)
  end
end

m.deinit = function() end

m.set_enabled = function(state)
  local name = m.position_name(m.pos)
  if name then mods.set_enabled(name, state, true) end
end

return m

