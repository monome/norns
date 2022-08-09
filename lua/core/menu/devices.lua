local keyboard = require 'core/keyboard'
local tabutil = require 'tabutil'

local m = {
  pos = 1,
  last_pos = 1,
  list = {"midi", "grid", "arc", "hid", "keyboard layout"},
}

m.len = #m.list

function m.refresh()
  m.options = {
    midi = {"none"},
    grid = {"none"},
    arc = {"none"},
    hid = {"none"},
  }
  m.options["keyboard layout"] = {}

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
  for _, device in pairs(hid.devices) do
    table.insert(m.options.hid, device.name)
  end
  for layout, _ in pairs(keyboard.keymap) do
    table.insert(m.options["keyboard layout"], layout)
  end
  table.sort(m.options["keyboard layout"])
end

local function set_len_for_list()
  if m.section == "midi" then
    m.len = 16
  else
    m.len = 4
  end
end

local function check_and_rename(dev_type,pos,name)
  for i = 1,#dev_type.vports do
    if i ~= pos and dev_type.vports[i].name == name then
      dev_type.vports[i].name = "none"
    end
  end
end

m.key = function(n,z)
  if m.mode == "type" then
    if n==2 and z==1 then
      _menu.set_page("SYSTEM")
    elseif n==3 and z==1 then
      m.section = m.list[m.pos]
      if m.section == "keyboard layout" then
        m.refresh()
        m.mode = "select"
        m.len = #m.options[m.section]
        m.pos = tabutil.key(m.options["keyboard layout"], keyboard.selected_map)
      else
        m.mode = "list"
        set_len_for_list()
        m.pos = 1
      end
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
      if m.section == "keyboard layout" then
        m.mode = "type"
        m.len = #m.list
        m.pos = 1
      else
        m.mode = "list"
        set_len_for_list()
        m.pos = m.last_pos
      end
      _menu.redraw()
    elseif n==3 and z==1 then
      local s = m.options[m.section][m.pos]
      local target_mode = "list"
      if m.section == "midi" then
        midi.vports[m.setpos].name = s
        check_and_rename(midi,m.setpos,s)
        midi.update_devices()
      elseif m.section == "grid" then
        grid.vports[m.setpos].name = s
        check_and_rename(grid,m.setpos,s)
        grid.update_devices()
      elseif m.section == "arc" then
        arc.vports[m.setpos].name = s
        check_and_rename(arc,m.setpos,s)
        arc.update_devices()
      elseif m.section == "hid" then
        hid.vports[m.setpos].name = s
        check_and_rename(hid,m.setpos,s)
        hid.update_devices()
      elseif m.section == "keyboard layout" then
        keyboard.set_map(s, true)
        target_mode = "type"
      end
      if target_mode == "type" then
        m.mode = "type"
        m.len = #m.list
        m.pos = 1
      else
        m.mode = "list"
        set_len_for_list()
        m.pos = m.last_pos
      end
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

local function redraw_select(do_show_all_if_fit, do_uppercase)
  screen.clear()

  if do_show_all_if_fit==nil then do_show_all_if_fit = true end
  if do_uppercase==nil then do_uppercase = false end
  local len = tabutil.count(m.options[m.section])

  if do_show_all_if_fit and len <= 6 then --  -> no scroll
    for i=1,len do
      local line = m.options[m.section][i]
      if do_uppercase then line = string.upper(line) end
      screen.level(i==m.pos and 15 or 4)
      screen.move(20,10*i)
      screen.text(line)
    end
  else --  -> scroll & selected item is always at pos 3
    for i=1,6 do
      if (i > 2 - m.pos + 1) and (i < m.len - m.pos + 3 + 1) then
        local line = m.options[m.section][i+m.pos-2 - 1]
        if do_uppercase then line = string.upper(line) end
        screen.level(i==3 and 15 or 4)
        screen.move(20,10*i)
        screen.text(line)
      end
    end
  end

  screen.update()
end

m.redraw = function()
  if m.mode == "select" then
    local do_show_all_if_fit = false
    local do_uppercase = (m.section == "keyboard layout")
    redraw_select(do_show_all_if_fit, do_uppercase)
    return
  end

  local y_offset = 0
  if not (m.mode == "type")
    and not (m.section == "midi" and m.mode == "list")
    and (4<m.pos) then
    y_offset = 10*(4-m.pos)
  elseif m.mode ~= "type" then
    y_offset = 20
  end
  screen.clear()
  if m.mode == "list" then
    screen.move(0,10)
    screen.level(4)
    screen.text(string.upper(m.section))
  end
  for i=1,m.len do
    screen.move(0,10*i+y_offset)
    if(i==m.pos) then
      screen.level(15)
    else
      screen.level(4)
    end
    if m.mode == "type" then
      screen.text(string.upper(m.list[i]) .. " >")
      if m.list[i] == "keyboard layout" then
        screen.move(127,10*i+y_offset)
        screen.text_right(string.upper(keyboard.selected_map))
      end
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
        screen.move(8,10*i+y_offset)
        screen.text(grid.vports[i].name)
      elseif m.section == "arc" then
        screen.text(i..".")
        screen.move(8,10*i+y_offset)
        screen.text(arc.vports[i].name)
      elseif m.section == "hid" then
        screen.text(i..".")
        screen.move(8,10*i+y_offset)
        screen.text(hid.vports[i].name)
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
