local textentry= require 'textentry'

local m = {
  pos = 1,
  list = {"RESET", "PASSWORD >", "BATTERY WARNING"},
  pages = {"RESET", "PASSWORD"}
}

m.key = function(n,z)
  if n==2 and z==1 then
    _menu.set_page("SYSTEM")
  elseif n==3 and z==1 then
    if m.pages[m.pos]=="PASSWORD" then
      textentry.enter(m.passdone, "", "new password:", m.passcheck)
    else
      _menu.set_page(m.pages[m.pos])
    end
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = util.clamp(m.pos + delta, 1, #m.list)
    _menu.redraw()
  elseif n==3 and m.list[m.pos]=="BATTERY WARNING" then
    norns.state.battery_warning = (delta>0) and 1 or 0
    screen.update = screen.update_default
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()
  for i=1,6 do
    if (i > 3 - m.pos) and (i < #m.list - m.pos + 4) then
      screen.move(0,10*i)
      local line = m.list[i+m.pos-3]
      if(i==3) then
        screen.level(15)
      else
        screen.level(4)
      end
      screen.text(line)
      if m.list[i+m.pos-3]=="BATTERY WARNING" then
	screen.move(128,10*i)
	screen.text_right(norns.state.battery_warning==1 and "on" or "off")
      end
    end
  end
  screen.update()
end

m.init = norns.none
m.deinit = norns.none

m.passcheck = function(txt)
  if txt ~= nil then
    if string.len(txt) < 8 then
      return ("remaining: "..8 - string.len(txt))
    elseif string.len(txt) > 63 then
      return ("too long")
    end
  end
end

m.passdone = function(txt)
  if txt ~= nil then
    if string.len(txt) >= 8 and string.len(txt) < 64 then
      local chpasswd_status = os.execute("echo 'we:"..txt.."' | sudo chpasswd")
      local smbpasswd_status = os.execute("printf '"..txt.."\n"..txt.."\n' | sudo smbpasswd -a we")
      local hotspotpasswd_status;
      local fd = io.open("home/we/norns/.system.hotspot_password", "w+")
      if fd then
        io.output(fd)
        io.write(txt)
        io.close(fd)
        hotspotpasswd_status = true
      end
      if chpasswd_status then print("ssh password changed") end
      if smbpasswd_status then print("samba password changed") end
      if hotspotpasswd_status then print("hotspot password changed, toggle WIFI off/on to take effect") end
    elseif string.len(txt) <= 8 then
      print("!! password must be at least 8 characters !!")
      print("!! password has not been changed !!")
    elseif string.len(txt) > 64 then
      print("!! password cannot be longer than 63 characters !!")
      print("!! password has not been changed !!")
    end
  end
  _menu.set_page("SETTINGS")
end


return m
