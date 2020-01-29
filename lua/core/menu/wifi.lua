local listselect = require 'listselect'
local textentry= require 'textentry'

local m = {
  pos = 0,
  list = {"off","hotspot","connect", "add", "del"},
  selected = 1,
  try = "",
  countdown = -1
}

m.len = #m.list

m.connect = function(x)
  if x ~= "cancel" then
    wifi.on(x)
  end
  _menu.redraw()
end

m.add = function(x)
  m.try = x
  if x ~= "cancel" then
    textentry.enter(m.passdone, "", "enter password:")
  end
  _menu.redraw()
end

m.del = function(x)
  if x ~= "cancel" then
    wifi.delete(x)
  end
  _menu.redraw()
end

m.passdone = function(txt)
  if txt ~= nil then
    print("adding " .. m.try .. txt)
    wifi.add(m.try, txt)
  end
  _menu.redraw()
end


m.key = function(n,z)
  if n==2 and z==1 then
    _menu.set_page("SYSTEM")
  elseif n==3 and z==1 then
    if m.pos == 0 then
      wifi.off()
    elseif m.pos == 1 then
      wifi.hotspot()
    elseif m.pos == 2 then
      wifi.update()
      listselect.enter(wifi.conn_list, m.connect)
    elseif m.pos == 3 then
      wifi.update()
      listselect.enter(m.ssid_list, m.add)
    elseif m.pos == 4 then
      wifi.update()
      listselect.enter(wifi.conn_list, m.del)
    end
  end
end

m.enc = function(n,delta)
  if n==2 then
    m.pos = m.pos + delta
    if m.pos > m.len - 1 then m.pos = m.len - 1
    elseif m.pos < 0 then m.pos = 0 end
    _menu.redraw()
  elseif n==3 and m.pos == 2 then
    m.selected = util.clamp(1,m.selected+delta,wifi.conn_count)
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()
  screen.level(4)

  screen.move(0,10)
  screen.text("STATUS: " .. wifi.status)
  screen.move(0,20)
  screen.text("NETWORK: " .. wifi.connection_name)
  screen.move(0,30)
  screen.text("IP: " .. wifi.ip)
  if wifi.ip and wifi.connection then
    if wifi.connection:is_wireless() then
      screen.move(0,40)
      screen.text("SIGNAL: " .. wifi.signal .. "dBm")
    end
  end

  local xp = {0,20,58,94,114}
  for i=1,m.len do
    screen.move(xp[i],60)
    local line = m.list[i]
    if(i==m.pos+1) then
      screen.level(15)
    else
      screen.level(4)
    end
    screen.text(string.upper(line))
  end

  screen.update()
end

m.init = function()
  -- screen enter notification
  screen.clear()
  screen.level(4)
  screen.move(64,40)
  screen.text_center("scanning...")
  screen.update()
  wifi.ensure_radio_is_on()
  m.ssid_list = wifi.ssids() or {}
  wifi.update()
  m.selected = 1
  _menu.timer.time = 3
  _menu.timer.count = -1
  _menu.timer.event = function()
    wifi.update()
    _menu.redraw()
  end
  _menu.timer:start()
end

m.deinit = function()
  _menu.timer:stop()
end

return m
