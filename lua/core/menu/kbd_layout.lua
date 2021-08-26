local keyboard = require 'core/keyboard'
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
  elseif n==3 and z==1 then
    m.select_entry(m.position_name(m.pos))
    _menu.redraw()
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = util.clamp(m.pos + delta, 0, m.len - 1)
    m.select_position(m.pos)
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()
  screen.level(15)

  for i=1,6 do
    if (i > 2 - m.pos) and (i < m.len - m.pos + 3) then
      local name = m.list[i+m.pos-2]
      local enabled = (name == keyboard.selected_map)
      local line = string.upper(name)

      local y = 10*i
      local line_level = 4
      if i==3 then
        line_level = 15
      end
      screen.level(line_level)

      -- selected item
      if _menu.m[name] then line = line .. " >" end
      screen.move(20,y)
      screen.text(line)

      -- change enabled state indicator
      screen.move(40,y)
      if enabled then
        screen.text("*")
      end
    end
  end
  screen.update()
end

m.init = function()
  m.list = {}
  for k, _ in pairs(keyboard.keymap) do
    table.insert(m.list, k)
  end
  table.sort(m.list)

  m.len = tab.count(m.list)
end

m.deinit = function() end

m.select_entry = function(name)
  if name then keyboard.set_map(name, true) end
end

return m
