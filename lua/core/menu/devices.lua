local m = {
  pos = 1,
  list = {"midi", "grid", "arc", "hid"},
}

m.len = #m.list

function m.refresh()
  m.options = {
    midi = {"none"},
    grid = {"none"},
    arc = {"none"},
    hid = {"none"},
  }
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
end

m.key = function(n,z)
  if m.mode == "type" then
    if n==2 and z==1 then
      norns.state.save()
      _menu.set_page("SYSTEM")
    elseif n==3 and z==1 then
      m.section = m.list[m.pos]
      m.mode = "list"
      m.len = 4
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
      m.refresh()
      m.mode = "select"
      m.setpos = m.pos
      m.len = #m.options[m.section]
      --tab.print(m.options[m.section])
      m.pos = 1
      _menu.redraw()
    end
  elseif m.mode == "select" then
    if n==2 and z==1 then
      m.mode = "list"
      m.len = 4
      m.pos = 1
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
      end
      m.mode = "list"
      m.len = 4
      m.pos = 1
      _menu.redraw()
    end
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = util.clamp(m.pos + delta, 1, m.len)
    _menu.redraw()
  end
end

m.redraw = function()
  local y_offset = 0
  if(4<m.pos) then
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
      screen.text(i..".")
      screen.move(8,10*i+20+y_offset)
      if m.section == "midi" then
        screen.text(midi.vports[i].name)
      elseif m.section == "grid" then
        screen.text(grid.vports[i].name)
      elseif m.section == "arc" then
        screen.text(arc.vports[i].name)
      elseif m.section == "hid" then
        screen.text(hid.vports[i].name)
      end
    elseif m.mode == "select" then
      screen.text(m.options[m.section][i])
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
