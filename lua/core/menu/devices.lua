local keyboard = require 'core/keyboard'

local m = {
  pos = 1,
  last_pos = 1,
  list = {"midi", "grid", "arc", "hid", "kbd layout"},
}

m.len = #m.list

function m.refresh()
  m.options = {
    midi = {"none"},
    grid = {"none"},
    arc = {"none"},
    hid = {"none"},
  }
  m.options["kbd layout"] = {}

  -- create midi list
  for _, device in pairs(midi.devices) do
    table.insert(m.options.midi, device.name)
  end
  for _, device in pairs(grid.devices) do
    table.insert(m.options.grid, device.name)
  end
  for _, device in pairs(arc.devices) do
    table.insert(m.options.arc, device.name)
  end
  for layout, _ in pairs(keyboard.keymap) do
    table.insert(m.options["kbd layout"], layout)
  end
  table.sort(m.options["kbd layout"])
end

local function set_len_for_section()
  if m.section == "midi" then
    m.len = 16
  elseif m.section == "kbd layout" then
    m.len = 1
  else
    m.len = 4
  end
end

m.key = function(n,z)
  if m.mode == "type" then
    if n==2 and z==1 then
      _menu.set_page("SYSTEM")
    elseif n==3 and z==1 then
      m.section = m.list[m.pos]
      m.mode = "list"
      set_len_for_section()
      m.pos = 1
      _menu.redraw()
    end
  elseif m.mode == "list" then
    if n==2 and z==1 then
      m.mode = "type"
      m.len = #m.list
      m.pos = 1
      _menu.redraw()
    elseif n==3 and z==1 then
      m.last_pos = m.pos
      m.refresh()
      m.mode = "select"
      m.setpos = m.pos
      m.len = #m.options[m.section]
      m.pos = 1
      _menu.redraw()
    end
  elseif m.mode == "select" then
    if n==2 and z==1 then
      m.mode = "list"
      set_len_for_section()
      m.pos = m.last_pos
      _menu.redraw()
    elseif n==3 and z==1 then
      local s = m.options[m.section][m.pos]
      if m.section == "midi" then
        midi.vports[m.setpos].name = s
        midi.update_devices()
      elseif m.section == "grid" then
        grid.vports[m.setpos].name = s
        grid.update_devices()
      elseif m.section == "arc" then
        arc.vports[m.setpos].name = s
        arc.update_devices()
      elseif m.section == "hid" then
        hid.vports[m.setpos].name = s
        hid.update_devices()
      elseif m.section == "kbd layout" then
        keyboard.set_map(s, true)
      end
      m.mode = "list"
      set_len_for_section()
      m.pos = m.last_pos
      _menu.redraw()
    end
  end
end

m.enc = function(n,delta)
  if n==2 then
    prev_pos = m.pos
    m.pos = util.clamp(m.pos + delta, 1, m.len)
    if prev_pos ~= m.pos then
      _menu.redraw()
    end
  end
end

m.redraw = function()
  local y_offset = 0
  if(4<m.pos) and not (m.section == "midi" and m.mode == "list") then
    y_offset = 10*(4-m.pos)
  end
  screen.clear()
  if m.mode == "list" then
    screen.move(0,10+y_offset)
    screen.level(4)
    screen.text(string.upper(m.section))
  end
  for i=1,m.len do
    screen.move(0,10*i+20+y_offset)
    if(i==m.pos) then
      screen.level(15)
    else
      screen.level(4)
    end
    if m.mode == "type" then
      screen.text(string.upper(m.list[i]) .. " >")
    elseif m.mode == "list" then
      if m.section == "midi" then
        for j = 1,4 do
          screen.move(0,10*j+20)
          if m.pos+(j-1) <= m.len then
            local line = m.pos+(j-1)..". "..midi.vports[m.pos+(j-1)].name
            if j == 1 then
              screen.level(15)
            else
              screen.level(3)
            end
            screen.text(line)
          end
        end
      elseif m.section == "grid" then
        screen.text(i..".")
        screen.move(8,10*i+20+y_offset)
        screen.text(grid.vports[i].name)
      elseif m.section == "arc" then
        screen.text(i..".")
        screen.move(8,10*i+20+y_offset)
        screen.text(arc.vports[i].name)
      elseif m.section == "hid" then
        screen.text(i..".")
        screen.move(8,10*i+20+y_offset)
        screen.text(hid.vports[i].name)
      elseif m.section == "kbd layout" then
        screen.level(3)
        screen.move(8,10*i+20+y_offset)
        screen.text("current:")
        screen.level(15)
        screen.move(43,10*i+20+y_offset)
        screen.text(keyboard.selected_map)
      end
    elseif m.mode == "select" then
      if m.section == "midi" then
        for j = 1,4 do
          screen.move(0,10*j+20)
          if m.options[m.section][m.pos+(j-1)] ~= nil then
            local line = m.options[m.section][m.pos+(j-1)]
            if j == 1 then
              screen.level(15)
            else
              screen.level(3)
            end
            screen.text(line)
          end
        end
      else
        screen.move(8,10*i+20+y_offset)
        screen.text(m.options[m.section][i])
      end
    end
  end
  screen.update()
end

m.init = function()
  m.pos = 1
  m.mode = "type"
  m.len = #m.list
end

m.deinit = function() end

return m
